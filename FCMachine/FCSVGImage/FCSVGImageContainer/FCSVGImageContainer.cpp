// FCSVGImageContainer.cpp
// Реализация потокобезопасного контейнера для результатов парсинга SVG
// Версия: 2.0.0 (Full Inkscape Support)

#include "FCSVGImageContainer.h"
#include <QMutexLocker>
#include <QtMath>

namespace
{
    qreal distance(const FC2DPoint &a, const FC2DPoint &b) noexcept
    {
        qreal dx = b.x() - a.x();
        qreal dy = b.y() - a.y();
        return qSqrt(dx * dx + dy * dy);
    }

    qreal distance(const FC3DPoint &a, const FC3DPoint &b) noexcept
    {
        qreal dx = b.x() - a.x();
        qreal dy = b.y() - a.y();
        return qSqrt(dx * dx + dy * dy);
    }

    [[maybe_unused]] qreal calculatePolygonArea(const QVector<FC2DPoint> &points) noexcept
    {
        if (points.size() < 3) return 0.0;
        qreal area = 0.0;
        int size = points.size();
        for (int i = 0; i < size; ++i) {
            int j = (i + 1) % size;
            area += points[i].x() * points[j].y();
            area -= points[j].x() * points[i].y();
        }
        return qAbs(area) * 0.5;
    }

    qreal calculatePolygonArea(const QVector<FC3DPoint> &points) noexcept
    {
        if (points.size() < 3) return 0.0;
        qreal area = 0.0;
        int size = points.size();
        for (int i = 0; i < size; ++i) {
            int j = (i + 1) % size;
            area += points[i].x() * points[j].y();
            area -= points[j].x() * points[i].y();
        }
        return qAbs(area) * 0.5;
    }

    [[maybe_unused]] qreal calculatePathLength(const QVector<FC2DPoint> &points) noexcept
    {
        if (points.size() < 2) return 0.0;
        qreal length = 0.0;
        for (int i = 1; i < points.size(); ++i)
            length += distance(points[i-1], points[i]);
        return length;
    }

    qreal calculatePathLength(const QVector<FC3DPoint> &points) noexcept
    {
        if (points.size() < 2) return 0.0;
        qreal length = 0.0;
        for (int i = 1; i < points.size(); ++i)
            length += distance(points[i-1], points[i]);
        return length;
    }

    void calculateBoundingBox(const QVector<FC3DPoint> &points, FC2DPoint &min, FC2DPoint &max) noexcept
    {
        if (points.isEmpty()) {
            min = FC2DPoint(0, 0);
            max = FC2DPoint(0, 0);
            return;
        }
        min = FC2DPoint(points.first().x(), points.first().y());
        max = FC2DPoint(points.first().x(), points.first().y());
        for (const auto &pt : points) {
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
}

// ============================================================================
// BinaryFigure
// ============================================================================

void FCSVGImageContainer::BinaryFigure::recalculateMetrics()
{
    if (points.isEmpty()) {
        boundingBoxMin = FC2DPoint(0, 0);
        boundingBoxMax = FC2DPoint(0, 0);
        length = 0.0;
        area = 0.0;
        return;
    }
    calculateBoundingBox(points, boundingBoxMin, boundingBoxMax);
    length = calculatePathLength(points);
    area = (isFilled && points.size() >= 3) ? calculatePolygonArea(points) : 0.0;
}

// ============================================================================
// BinaryLayer
// ============================================================================

quint32 FCSVGImageContainer::BinaryLayer::appendFigure(const BinaryFigure &figure)
{
    BinaryFigure fig = figure;
    fig.layerIndex = index;
    figures.append(fig);
    return static_cast<quint32>(figures.size()) - 1;
}

// ============================================================================
// FCSVGImageContainer
// ============================================================================

FCSVGImageContainer::FCSVGImageContainer(QObject *parent)
    : QObject(parent),
      _layers(),
      _metadata(),
      _lock(QReadWriteLock::Recursive),
//      _readyState(0),
      _writeInProgress(0),
      _figureCounter(1),
      _viewBox(),
      _state{FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::None}  // ✅ ИСПРАВЛЕНО
{
    setObjectName(QStringLiteral("SVGContainer"));
}

FCSVGImageContainer::FCSVGImageContainer(const QString &name, QObject *parent)
    : FCSVGImageContainer(parent)
{
    setObjectName(name);
}

FCSVGImageContainer::FCSVGImageContainer(const FCSVGImageContainer &other, QObject *parent)
    : QObject(parent),
      _layers(other._layers),
      _metadata(other._metadata),
      _lock(),
//      _readyState(other._readyState.loadRelaxed()),
      _writeInProgress(0),
      _figureCounter(other._figureCounter),
      _viewBox(other._viewBox),
      _state(other._state)  // ✅ Копирование состояния
{
    setObjectName(other.objectName());
}

FCSVGImageContainer::~FCSVGImageContainer() = default;

// ============================================================================
// УПРАВЛЕНИЕ ЗАПИСЬЮ
// ============================================================================

bool FCSVGImageContainer::beginWrite()
{
    if (_lock.tryLockForWrite()) {
        _writeInProgress.storeRelaxed(1);
        return true;
    }
    return false;
}

void FCSVGImageContainer::endWrite()
{
    calculateStatistics();
    _writeInProgress.storeRelaxed(0);
    _lock.unlock();
    emit writeProgress(100);
}

quint32 FCSVGImageContainer::appendLayer(const BinaryLayer &layer)
{
    return addLayer(layer);
}

quint32 FCSVGImageContainer::addLayer(const BinaryLayer &layer)
{
    QWriteLocker lock(&_lock);
    return addLayerInternal(layer);
}

quint32 FCSVGImageContainer::addLayerInternal(const BinaryLayer &layer)
{
    BinaryLayer newLayer = layer;
    newLayer.index = static_cast<quint32>(_layers.size());
    _layers.append(newLayer);
    _metadata.layerCount = static_cast<quint32>(_layers.size());
    emit layerAdded(newLayer.index);
    return newLayer.index;
}

bool FCSVGImageContainer::appendFigure(quint32 layerIndex, const BinaryFigure &figure)
{
    QWriteLocker lock(&_lock);
    if (layerIndex >= static_cast<quint32>(_layers.size()))
        return false;

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

FCSVGImageContainer::BinaryLayer &FCSVGImageContainer::layerRef(quint32 layerIndex)
{
    validateLayerIndex(layerIndex);
    return _layers[layerIndex];
}

FCSVGImageContainer::BinaryLayer &FCSVGImageContainer::defaultLayer()
{
    if (_layers.isEmpty()) {
        BinaryLayer defaultLayer;
        defaultLayer.index = 0;
        defaultLayer.name = QStringLiteral("default");
        defaultLayer.isVisible = true;
        addLayerInternal(defaultLayer);
    }
    return _layers.first();
}

void FCSVGImageContainer::setMetadata(const Metadata &metadata)
{
    QWriteLocker lock(&_lock);
    _metadata = metadata;
}

void FCSVGImageContainer::setViewBox(const FC2DSize &viewBox)
{
    QWriteLocker lock(&_lock);
    _viewBox = viewBox;
    _metadata.imageSize = viewBox;
}

void FCSVGImageContainer::clear()
{
    QWriteLocker lock(&_lock);
    _layers.clear();
    _metadata = Metadata();
    _figureCounter = 1;
    _viewBox = FC2DSize();
//    _readyState.storeRelaxed(0);
    _writeInProgress.storeRelaxed(0);

    // ✅ Сброс состояния через FCStateT<>
    _state.set(FCReadyState::NotReady);
}

void FCSVGImageContainer::setReady()
{
//    _readyState.storeRelaxed(1);
    _state.set(FCReadyState::Ready);  // ✅ Обновление состояния
    emit condition(_state);
}

// ЧТЕНИЕ ДАННЫХ
// ============================================================================

FCSVGImageContainer::BinaryLayer FCSVGImageContainer::layer(quint32 index) const
{
    QReadLocker lock(&_lock);
    validateLayerIndex(index);
    return _layers.at(index);
}

QVector<FCSVGImageContainer::BinaryLayer> FCSVGImageContainer::layers() const
{
    QReadLocker lock(&_lock);
    return _layers;
}

FCSVGImageContainer::BinaryFigure FCSVGImageContainer::figure(quint32 layerIndex, quint32 figureIndex) const
{
    QReadLocker lock(&_lock);
    validateLayerIndex(layerIndex);
    const auto &layer = _layers.at(layerIndex);
    if (figureIndex >= static_cast<quint32>(layer.figures.size()))
        return BinaryFigure();
    return layer.figures.at(figureIndex);
}

QVector<FCSVGImageContainer::BinaryFigure> FCSVGImageContainer::figures(quint32 layerIndex) const
{
    QReadLocker lock(&_lock);
    validateLayerIndex(layerIndex);
    return _layers.at(layerIndex).figures;
}

FCSVGImageContainer::Metadata FCSVGImageContainer::metadata() const noexcept
{
    return _metadata;
}

bool FCSVGImageContainer::isValid() const noexcept
{
    // ✅ ИСПРАВЛЕНО: FCErrorType::None вместо ErrorType::NoError
    if (!isReady() || !_state.is(FCErrorState::None))
        return false;
    if (_layers.isEmpty())
        return false;

    QReadLocker lock(&_lock);
    for (const auto &layer : _layers) {
        if (layer.isVisible && !layer.figures.isEmpty())
            return true;
    }
    return false;
}

quint64 FCSVGImageContainer::memorySize() const noexcept
{
    QReadLocker lock(&_lock);
    quint64 size = 0;
    size += static_cast<quint64>(_layers.capacity()) * sizeof(BinaryLayer);
    for (const auto &layer : _layers) {
        size += static_cast<quint64>(layer.figures.capacity()) * sizeof(BinaryFigure);
        for (const auto &fig : layer.figures) {
            size += static_cast<quint64>(fig.points.capacity()) * sizeof(FC3DPoint);
            size += static_cast<quint64>(fig.pointTypes.capacity()) * sizeof(quint8);
            if (!fig.fillPath.isEmpty())
                size += static_cast<quint64>(fig.fillPath.capacity()) * sizeof(FC2DPoint);
            size += static_cast<quint64>(fig.pathData.capacity()) * sizeof(QChar);
        }
        size += static_cast<quint64>(layer.name.capacity()) * sizeof(QChar);
        size += static_cast<quint64>(layer.inkscapeId.capacity()) * sizeof(QChar);
    }
    size += sizeof(Metadata);
    size = static_cast<quint64>(static_cast<qreal>(size) * 1.1);
    return size;
}

QSharedPointer<FCSVGImageContainer> FCSVGImageContainer::snapshot() const
{
    auto snap = QSharedPointer<FCSVGImageContainer>::create(objectName(), nullptr);
    {
        QReadLocker lock(&_lock);
        snap->_layers = _layers;
        snap->_metadata = _metadata;
        snap->_viewBox = _viewBox;
        snap->_figureCounter = _figureCounter;
        snap->_state = _state;  // ✅ Копирование состояния
    }
    snap->state().is(FCReadyState::Ready);
    snap->_writeInProgress.storeRelaxed(0);
    return snap;
}

qint32 FCSVGImageContainer::findLayerByName(const QString &name) const
{
    QReadLocker lock(&_lock);
    for (const auto &layer : _layers) {
        if (layer.name == name)
            return static_cast<qint32>(layer.index);
    }
    return -1;
}

QVector<QPair<quint32, quint32>> FCSVGImageContainer::findFiguresByColor(const QColor &color, quint8 tolerance) const
{
    QVector<QPair<quint32, quint32>> result;
    QReadLocker lock(&_lock);
    quint32 li = 0;
    for (const auto &layer : _layers) {
        if (!layer.isVisible) continue;
        quint32 fi = 0;
        for (const auto &fig : layer.figures) {
            if (colorsMatch(fig.color, color, tolerance))
                result.append(qMakePair(li, fi));
            ++fi;
        }
        ++li;
    }
    return result;
}

void FCSVGImageContainer::validateLayerIndex(quint32 index) const
{
    Q_ASSERT_X(index < static_cast<quint32>(_layers.size()),
               "FCSVGImageContainer::validateLayerIndex",
               qPrintable(QStringLiteral("Index %1 out of range [0; %2)").arg(index).arg(_layers.size())));
}

void FCSVGImageContainer::calculateStatistics()
{
    quint32 totalFigures = 0;
    quint64 totalPoints = 0;
    qreal totalLength = 0.0;
    qreal totalArea = 0.0;

    for (const auto &layer : _layers) {
        if (!layer.isVisible) continue;
        for (const auto &fig : layer.figures) {
            totalFigures++;
            totalPoints += static_cast<quint64>(fig.points.size());
            totalLength += fig.length;
            totalArea += fig.area;
        }
    }

    _metadata.figureCount = totalFigures;
    _metadata.totalPoints = totalPoints;

    if (totalArea > 0.0) {
        constexpr qreal avgThickness = 0.4;
        qreal coverage = (totalLength * avgThickness) / totalArea;
        _metadata.inkCoverage = qBound(0.0, coverage, 1.0);
    } else {
        _metadata.inkCoverage = 0.0;
    }

    if (totalFigures > 0) {
        for (const auto &layer : _layers) {
            if (!layer.isVisible || layer.figures.isEmpty()) continue;
            _metadata.dominantColor = layer.figures.first().color;
            break;
        }
    }
}

quint32 FCSVGImageContainer::generateFigureId()
{
    return _figureCounter++;
}

FCSVGImageContainer& FCSVGImageContainer::operator=(const FCSVGImageContainer &other)
{
    if (this == &other) return *this;

    QReadLocker otherLock(&other._lock);
    QWriteLocker thisLock(&_lock);

    _layers = other._layers;
    _metadata = other._metadata;
    _viewBox = other._viewBox;
    _figureCounter = other._figureCounter;
//    _readyState.storeRelaxed(other._readyState.loadRelaxed());
    _writeInProgress.storeRelaxed(0);
    _state = other._state;  // ✅ Копирование состояния

    return *this;
}
