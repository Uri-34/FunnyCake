// FCImageBinaryContainer.cpp
// Потокобезопасный контейнер для хранения результатов парсинга векторной графики
// Версия: 2.1.0 (State-Driven, Thread-Safe)

#include "FCImageBinaryContainer.h"
#include <QMutexLocker>
#include <QtMath>
#include <algorithm>

namespace
{
    // Унифицированный расчёт 2D-расстояния. Работает для FC2DPoint и FC3DPoint.
    // Шаблон исключает ошибки перегрузок и дублирование кода.
    template<typename PointT> qreal distance(const PointT &a, const PointT &b) noexcept
    {
        qreal dx = b.x() - a.x();
        qreal dy = b.y() - a.y();
        return qSqrt(dx * dx + dy * dy);
    }

    // [[maybe_unused]] оставляем, чтобы компилятор не ругался на неиспользуемые специализации
    template qreal distance<FC2DPoint>(const FC2DPoint&, const FC2DPoint&) noexcept;
    template qreal distance<FC3DPoint>(const FC3DPoint&, const FC3DPoint&) noexcept;

    [[maybe_unused]] qreal calculatePolygonArea(const QVector<FC2DPoint> &points) noexcept
    {
        if(points.size() < 3)
        {
            return 0.0;
        }

        qreal area = 0.0;
        int size = points.size();
        for(int i = 0; i < size; ++i)
        {
            int j = (i + 1) % size;
            area += points[i].x() * points[j].y();
            area -= points[j].x() * points[i].y();
        }

        return qAbs(area) * 0.5;
    }

    qreal calculatePolygonArea(const QVector<FC3DPoint> &points) noexcept
    {
        if(points.size() < 3)
        {
            return 0.0;
        }

        qreal area = 0.0;
        int size = points.size();
        for(int i = 0; i < size; ++i)
        {
            int j = (i + 1) % size;
            area += points[i].x() * points[j].y();
            area -= points[j].x() * points[i].y();
        }
        return qAbs(area) * 0.5;
    }

    [[maybe_unused]] qreal calculatePathLength(const QVector<FC2DPoint> &points) noexcept
    {
        if(points.size() < 2)
        {
            return 0.0;
        }

        qreal length = 0.0;
        for(int i = 1; i < points.size(); ++i)
        {
            length += distance(points[i-1], points[i]);
        }

        return length;
    }

    qreal calculatePathLength(const QVector<FC3DPoint> &points) noexcept
    {
        if(points.size() < 2)
        {
            return 0.0;
        }

        qreal length = 0.0;
        for(int i = 1; i < points.size(); ++i)
        {
            length += distance(points[i-1], points[i]);
        }

        return length;
    }

    void calculateBoundingBox(const QVector<FC3DPoint> &points, FC2DPoint &min, FC2DPoint &max) noexcept
    {
        if(points.isEmpty())
        {
            min = FC2DPoint(0, 0);
            max = FC2DPoint(0, 0);
            return;
        }
        min = FC2DPoint(points.first().x(), points.first().y());
        max = min;
        for(const auto &pt : points)
        {
            if (pt.x() < min.x()) min.setX(pt.x());
            if (pt.y() < min.y()) min.setY(pt.y());
            if (pt.x() > max.x()) max.setX(pt.x());
            if (pt.y() > max.y()) max.setY(pt.y());
        }
    }

    bool colorsMatch(const QColor &a, const QColor &b, quint8 tolerance) noexcept
    {
        return qAbs(a.red() - b.red()) <= tolerance &&
               qAbs(a.green() - b.green()) <= tolerance &&
               qAbs(a.blue() - b.blue()) <= tolerance;
    }
} // namespace

// КОНСТРУКТОРЫ / ДЕСТРУКТОР
FCImageBinaryContainer::FCImageBinaryContainer(const QString &name, QObject *parent)
    : FCService(name, parent),
      _lock(QReadWriteLock::Recursive),
      _writeInProgress{0},
      _figureCounter{1}
{
    setObjectName(name);
}

FCImageBinaryContainer::~FCImageBinaryContainer() = default;

// УПРАВЛЕНИЕ ЗАПИСЬЮ
bool FCImageBinaryContainer::beginWrite()
{
    if(_lock.tryLockForWrite())
    {
        _writeInProgress.storeRelaxed(1);
        return true;
    }
    return false;
}

void FCImageBinaryContainer::endWrite()
{
    if(_writeInProgress.loadRelaxed() == 1)
    {
        calculateStatistics();
        _writeInProgress.storeRelaxed(0);
        _lock.unlock();
        emit progress(100);
    }
}

quint32 FCImageBinaryContainer::appendLayer(const BinaryLayer &layer) { return addLayer(layer); }
quint32 FCImageBinaryContainer::addLayer(const BinaryLayer &layer)
{
    QWriteLocker lock(&_lock);
    return addLayerInternal(layer);
}

quint32 FCImageBinaryContainer::addLayerInternal(const BinaryLayer &layer)
{
    BinaryLayer newLayer = layer;
    newLayer.index = static_cast<quint32>(_layers.size());
    _layers.append(newLayer);
    _metadata.layerCount = static_cast<quint32>(_layers.size());
    emit layerAdded(newLayer.index);
    return newLayer.index;
}

bool FCImageBinaryContainer::appendFigure(quint32 layerIndex, const BinaryFigure &figure)
{
    QWriteLocker lock(&_lock);
    if(layerIndex >= static_cast<quint32>(_layers.size()))
    {
        return false;
    }

    BinaryFigure fig = figure;
    fig.id = generateFigureId();
    fig.layerIndex = layerIndex;
    fig.recalculateMetrics();

    _layers[layerIndex].figures.append(fig);
    _metadata.figureCount++;
    _metadata.totalPoints += static_cast<quint64>(fig.points.size());

    emit figureAdded(layerIndex, static_cast<quint32>(_layers[layerIndex].figures.size()) - 1);

    return true;
}

FCImageBinaryContainer::BinaryLayer &FCImageBinaryContainer::layerRef(quint32 layerIndex)
{
    validateLayerIndex(layerIndex);
    return _layers[layerIndex];
}

FCImageBinaryContainer::BinaryLayer &FCImageBinaryContainer::defaultLayer()
{
    if(_layers.isEmpty())
    {
        BinaryLayer def;
        def.index = 0;
        def.name = QStringLiteral("default");
        def.isVisible = true;
        addLayerInternal(def);
    }

    return _layers.first();
}

void FCImageBinaryContainer::setMetadata(const Metadata &metadata)
{
    QWriteLocker lock(&_lock);
    _metadata = metadata;
}

void FCImageBinaryContainer::setViewBox(const FC2DSize &viewBox)
{
    QWriteLocker lock(&_lock);
    _viewBox = viewBox;
    _metadata.imageSize = viewBox;
}

void FCImageBinaryContainer::clear()
{
    QWriteLocker lock(&_lock);
    _layers.clear();
    _metadata = Metadata();
    _viewBox = FC2DSize();
    _figureCounter.storeRelaxed(1);
    _writeInProgress.storeRelaxed(0);
    _state.set(FCReadyState::NotReady);
}

//void FCImageBinaryContainer::setReady()
//{
//    QWriteLocker lock(&_lock);
//    emit condition(_state.set(FCReadyState::Ready));
//}

// ЧТЕНИЕ ДАННЫХ (const-методы)
FCImageBinaryContainer::BinaryLayer FCImageBinaryContainer::layer(quint32 index) const
{
    QReadLocker lock(&_lock);
    validateLayerIndex(index);
    return _layers.at(index);
}

QVector<FCImageBinaryContainer::BinaryLayer> FCImageBinaryContainer::layers() const
{
    QReadLocker lock(&_lock);
    return _layers;
}

FCImageBinaryContainer::BinaryFigure FCImageBinaryContainer::figure(quint32 layerIndex, quint32 figureIndex) const
{
    QReadLocker lock(&_lock);
    validateLayerIndex(layerIndex);
    const auto &layer = _layers.at(layerIndex);
    if(figureIndex >= static_cast<quint32>(layer.figures.size()))
    {
        return BinaryFigure();
    }
    return layer.figures.at(figureIndex);
}

QVector<FCImageBinaryContainer::BinaryFigure> FCImageBinaryContainer::figures(quint32 layerIndex) const
{
    QReadLocker lock(&_lock);
    validateLayerIndex(layerIndex);
    return _layers.at(layerIndex).figures;
}

FCImageBinaryContainer::Metadata FCImageBinaryContainer::metadata() const noexcept
{
    QReadLocker lock(&_lock);
    return _metadata;
}

//bool FCImageBinaryContainer::isValid() const noexcept
//{
//    QReadLocker lock(&_lock);
//    if(!_state.is(FCReadyState::Ready) || !_state.is(FCErrorType::None) || _layers.isEmpty())
//    {
//        return false;
//    }
//    for(const auto &layer : _layers)
//    {
//        if(layer.isVisible && !layer.figures.isEmpty())
//        {
//            return true;
//        }
//    }
//    return false;
//}

quint64 FCImageBinaryContainer::memorySize() const noexcept
{
    QReadLocker lock(&_lock);
    quint64 size = sizeof(_layers) + sizeof(_metadata) + sizeof(_viewBox) + sizeof(_state);

    for(const auto &layer : _layers)
    {
        size += static_cast<quint64>(layer.figures.capacity()) * sizeof(BinaryFigure);
        for(const auto &fig : layer.figures)
        {
            size += static_cast<quint64>(fig.points.capacity()) * sizeof(FC3DPoint);
            size += static_cast<quint64>(fig.pathData.capacity()) * sizeof(QChar);
        }
        size += static_cast<quint64>(layer.name.capacity()) * sizeof(QChar);
        size += static_cast<quint64>(layer.inkscapeId.capacity()) * sizeof(QChar);
    }

    return size * 11 / 10; // ~10% overhead for allocator metadata
}

// УТИЛИТЫ
QSharedPointer<FCImageBinaryContainer> FCImageBinaryContainer::snapshot() const
{
    auto snap = QSharedPointer<FCImageBinaryContainer>::create(objectName(), nullptr);
    {
        QReadLocker lock(&_lock);
        snap->_layers = _layers;
        snap->_metadata = _metadata;
        snap->_viewBox = _viewBox;
        snap->_figureCounter.storeRelaxed(_figureCounter.loadRelaxed());
        snap->_state = _state;
    }
    snap->_state.set(FCReadyState::Ready);
    snap->_writeInProgress.storeRelaxed(0);
    return snap;
}

qint32 FCImageBinaryContainer::findLayerByName(const QString &name) const
{
    QReadLocker lock(&_lock);
    qint32 index = 0;
    for(const auto &layer : _layers)
    {
        if(layer.name == name)
        {
            return index;
        }
        ++index;
    }
    return -1;
}

QVector<QPair<quint32, quint32>> FCImageBinaryContainer::findFiguresByColor(
    const QColor &color, quint8 tolerance) const
{
    QVector<QPair<quint32, quint32>> result;
    QReadLocker lock(&_lock);

    // ✅ Было (ошибка):
    // for (quint32 li = 0; li < _layers.size(); ++li)

    // ✅ Стало (исправлено):
    for(qsizetype li = 0; li < _layers.size(); ++li)
    {
        if(!_layers.at(li).isVisible)
        {
            continue;
        }
        // ✅ Аналогично для вложенного цикла:
        for(qsizetype fi = 0; fi < _layers.at(li).figures.size(); ++fi)
        {
            if(colorsMatch(_layers.at(li).figures.at(fi).color, color, tolerance))
            {
                // Приводим обратно к quint32 для результата (индексы всегда неотрицательные)
                result.append(qMakePair(static_cast<quint32>(li), static_cast<quint32>(fi)));
            }
        }
    }
    return result;
}

FCImageBinaryContainer& FCImageBinaryContainer::operator=(const FCImageBinaryContainer &other)
{
    if(this == &other) return *this;
    QReadLocker oLock(&other._lock);
    QWriteLocker tLock(&_lock);

    _layers = other._layers;
    _metadata = other._metadata;
    _viewBox = other._viewBox;
    _figureCounter.storeRelaxed(other._figureCounter.loadRelaxed());
    _writeInProgress.storeRelaxed(0);
    _state = other._state;
    return *this;
}

// ЧАСТНЫЕ МЕТОДЫ И МЕТОДЫ СТРУКТУР
void FCImageBinaryContainer::validateLayerIndex(quint32 index) const
{
    Q_ASSERT_X(index < static_cast<quint32>(_layers.size()),
               "FCImageBinaryContainer::validateLayerIndex",
               qPrintable(QStringLiteral("Index %1 out of range [0; %2)").arg(index).arg(_layers.size())));
}

void FCImageBinaryContainer::calculateStatistics()
{
    quint32 totalFigures = 0;
    quint64 totalPoints = 0;
    qreal totalLength = 0.0;
    qreal totalArea = 0.0;

    for(auto &layer : _layers) {
        if(!layer.isVisible) continue;
        for(auto &fig : layer.figures)
        {
            ++totalFigures;
            totalPoints += static_cast<quint64>(fig.points.size());
            totalLength += fig.length;
            totalArea += fig.area;
        }
    }

    _metadata.figureCount = totalFigures;
    _metadata.totalPoints = totalPoints;

    if(totalArea > 0.0 && totalLength > 0.0)
    {
        constexpr qreal avgThickness = 0.4;
        _metadata.inkCoverage = qBound(0.0, (totalLength * avgThickness) / totalArea, 1.0);
    }
    else
    {
        _metadata.inkCoverage = 0.0;
    }

    if(totalFigures > 0)
    {
        for(auto &layer : _layers)
        {
            if(layer.isVisible && !layer.figures.isEmpty())
            {
                _metadata.dominantColor = layer.figures.first().color;
                break;
            }
        }
    }
}

quint32 FCImageBinaryContainer::generateFigureId()
{
    return _figureCounter.fetchAndAddOrdered(1);
}

void FCImageBinaryContainer::BinaryFigure::recalculateMetrics()
{
    if(points.isEmpty())
    {
        boundingBoxMin = {0, 0};
        boundingBoxMax = {0, 0};
        length = 0.0;
        area = 0.0;

        return;
    }
    calculateBoundingBox(points, boundingBoxMin, boundingBoxMax);
    length = calculatePathLength(points);
    area = (isFilled && points.size() >= 3) ? calculatePolygonArea(points) : 0.0;
}

quint32 FCImageBinaryContainer::BinaryLayer::appendFigure(const BinaryFigure &figure)
{
    BinaryFigure fig = figure;
    fig.layerIndex = index;
    figures.append(fig);
    return static_cast<quint32>(figures.size()) - 1;
}
