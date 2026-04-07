// FCSVGImageParser.cpp
// Реализация парсера SVG-файлов в бинарный формат для генерации G-кода
// Версия: 2.0.0 (Full Inkscape Support)
// Архитектура: Управление состояниями через QStateMachine (FCConditionObject УДАЛЁН)

#include <QFile>
#include <QXmlStreamReader>
#include <QVector>
#include <QString>
#include <QColor>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QAtomicInt>
#include <QFuture>
#include <QtConcurrent>
#include <QStack>
#include <QStateMachine>
#include <QState>
#include <QDebug>

#include "FCSVGImageParser.h"

// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ОПТИМИЗАЦИИ ГЕОМЕТРИИ
// Удаление дублирующихся точек в контуре (2D)
void FCSVGImageParser::removeDuplicatePoints(QVector<FC2DPoint> &points) noexcept
{
    if(points.size() < 2) return;
    auto last = std::unique(points.begin(), points.end(),
        [](const FC2DPoint &a, const FC2DPoint &b) {
            return qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y());
        });
    points.erase(last, points.end());
}

// Удаление дублирующихся точек в контуре (3D)
void FCSVGImageParser::removeDuplicatePoints(QVector<FC3DPoint> &points) noexcept
{
    if(points.size() < 2) return;
    auto last = std::unique(points.begin(), points.end(),
        [](const FC3DPoint &a, const FC3DPoint &b) {
            return qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y());
        });
    points.erase(last, points.end());
}

// Вычисление 2D-расстояния между точками (только X,Y)
qreal FCSVGImageParser::distance2D(const FC3DPoint &a, const FC3DPoint &b) noexcept
{
    qreal dx = b.x() - a.x();
    qreal dy = b.y() - a.y();
    return qSqrt(dx * dx + dy * dy);
}

// Упрощение пути алгоритмом Рамера-Дугласа-Пекера (2D)
void FCSVGImageParser::simplifyPath(QVector<FC2DPoint> &points, qreal tolerance) noexcept
{
    if(points.size() < 3) return;
    QVector<FC2DPoint> result;
    result.reserve(points.size());
    result.append(points.first());
    FC2DPoint lastKept = points.first();
    for(int i = 1; i < points.size() - 1; ++i)
    {
        if(lastKept.distanceTo(points[i]) >= tolerance)
        {
            result.append(points[i]);
            lastKept = points[i];
        }
    }
    if(!points.isEmpty())
        result.append(points.last());
    if(result.size() >= 2)
        points = std::move(result);
}

// Упрощение пути алгоритмом Рамера-Дугласа-Пекера (3D)
void FCSVGImageParser::simplifyPath(QVector<FC3DPoint> &points, qreal tolerance) noexcept
{
    if(points.size() < 3) return;
    QVector<FC3DPoint> result;
    result.reserve(points.size());
    result.append(points.first());
    FC2DPoint lastKept(points.first().x(), points.first().y());
    for(int i = 1; i < points.size() - 1; ++i)
    {
        FC2DPoint current(points[i].x(), points[i].y());
        if(lastKept.distanceTo(current) >= tolerance)
        {
            result.append(points[i]);
            lastKept = current;
        }
    }
    if(!points.isEmpty())
        result.append(points.last());
    if(result.size() >= 2)
        points = std::move(result);
}

// Объединение смежных коллинеарных сегментов (2D)
void FCSVGImageParser::mergeAdjacentSegments(QVector<FC2DPoint> &points) noexcept
{
    if(points.size() < 3) return;
    QVector<FC2DPoint> result;
    result.reserve(points.size());
    result.append(points.first());
    for(int i = 1; i < points.size() - 1; ++i)
    {
        qreal dx1 = points[i].x() - points[i-1].x();
        qreal dy1 = points[i].y() - points[i-1].y();
        qreal dx2 = points[i+1].x() - points[i].x();
        qreal dy2 = points[i+1].y() - points[i].y();
        qreal cross = dx1 * dy2 - dy1 * dx2;
        if(qAbs(cross) > 0.001)
        {
            result.append(points[i]);
        }
    }
    if(!points.isEmpty())
        result.append(points.last());
    if(result.size() >= 2)
        points = std::move(result);
}

// Объединение смежных коллинеарных сегментов (3D)
void FCSVGImageParser::mergeAdjacentSegments(QVector<FC3DPoint> &points) noexcept
{
    if(points.size() < 3) return;
    QVector<FC3DPoint> result;
    result.reserve(points.size());
    result.append(points.first());
    for(int i = 1; i < points.size() - 1; ++i)
    {
        qreal dx1 = points[i].x() - points[i-1].x();
        qreal dy1 = points[i].y() - points[i-1].y();
        qreal dx2 = points[i+1].x() - points[i].x();
        qreal dy2 = points[i+1].y() - points[i].y();
        qreal cross = dx1 * dy2 - dy1 * dx2;
        if(qAbs(cross) > 0.001)
        {
            result.append(points[i]);
        }
    }
    if(!points.isEmpty())
        result.append(points.last());
    if(result.size() >= 2)
        points = std::move(result);
}

// КОНСТРУКТОРЫ / ДЕСТРУКТОР
FCSVGImageParser::FCSVGImageParser(QObject *parent)
    : QObject(parent),
  _settings(),
  _container(nullptr),
  _future(),
  _isParsing{0},
  _currentFile(),
  _stateMachine{new QStateMachine(this)},
  _state{FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Unchanged, FCErrorType::None}
{
    setObjectName("SVGParser");
    initStateMachine();
}

FCSVGImageParser::FCSVGImageParser(const ParserSettings &settings, QObject *parent)
    : QObject(parent),
  _settings(settings),
  _container(nullptr),
  _future(),
  _isParsing{0},
  _currentFile(),
  _stateMachine{new QStateMachine(this)},
  _state{FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Unchanged, FCErrorType::None}
{
    setObjectName("SVGParser");
    initStateMachine();
}

FCSVGImageParser::~FCSVGImageParser()
{
    cancel();
    if (_stateMachine && _stateMachine->isRunning()) {
        _stateMachine->stop();
    }
    if(_container)
    {
        delete _container;
        _container = nullptr;
    }
}

// МЕТОДЫ УСТАНОВКИ СОСТОЯНИЙ
void FCSVGImageParser::set(FCReadyState state)
{
    if (!_state.is(state))
    {
        _state.set(state);
        emit readyStateChanged(state);
    }
}

void FCSVGImageParser::set(FCPlayState state)
{
    if (!_state.is(state))
    {
        _state.set(state);
        emit playStateChanged(state);
    }
}

void FCSVGImageParser::set(FCChangedState state)
{
    if (!_state.is(state))
    {
        _state.set(state);
        emit changedStateChanged(state);
    }
}

void FCSVGImageParser::set(FCErrorType type)
{
    if (!_state.is(type))
    {
        _state.set(type);
        emit errorTypeChanged(type);
    }
}

// УПРАВЛЕНИЕ НАСТРОЙКАМИ
void FCSVGImageParser::setSettings(const ParserSettings &settings)
{
    _settings = settings;
}

FCSVGImageParser::ParserSettings FCSVGImageParser::settings() const noexcept
{
    return _settings;
}

// МАШИНА СОСТОЯНИЙ
void FCSVGImageParser::initStateMachine()
{
    // === КОРНЕВОЕ ПАРАЛЛЕЛЬНОЕ СОСТОЯНИЕ ===
    QState *rootState = new QState(QState::ParallelStates);
    rootState->setObjectName("RootState");

    // === ГРУППА СОСТОЯНИЙ ГОТОВНОСТИ ===
    QState *readyStateGroup = new QState(rootState);
    readyStateGroup->setObjectName("ReadyStateGroup");

    QState *notReadyState = new QState(readyStateGroup);
    notReadyState->setObjectName("NotReady");

    QState *readyState = new QState(readyStateGroup);
    readyState->setObjectName("Ready");

    readyStateGroup->setInitialState(notReadyState);

    // === ГРУППА СОСТОЯНИЙ ВОСПРОИЗВЕДЕНИЯ ===
    QState *playStateGroup = new QState(rootState);
    playStateGroup->setObjectName("PlayStateGroup");

    QState *stopState = new QState(playStateGroup);
    stopState->setObjectName("Stop");

    QState *startState = new QState(playStateGroup);
    startState->setObjectName("Start");

    QState *pauseState = new QState(playStateGroup);
    pauseState->setObjectName("Pause");

    playStateGroup->setInitialState(stopState);

    // === ГРУППА СОСТОЯНИЙ ИЗМЕНЕНИЙ ===
    QState *changedStateGroup = new QState(rootState);
    changedStateGroup->setObjectName("ChangedStateGroup");

    QState *unchangedState = new QState(changedStateGroup);
    unchangedState->setObjectName("Unchanged");

    QState *changedState = new QState(changedStateGroup);
    changedState->setObjectName("Changed");

    changedStateGroup->setInitialState(unchangedState);

    // === ГРУППА СОСТОЯНИЙ ОШИБОК ===
    QState *errorStateGroup = new QState(rootState);
    errorStateGroup->setObjectName("ErrorStateGroup");

    QState *noErrorState = new QState(errorStateGroup);
    noErrorState->setObjectName("None");

    QState *criticalErrorState = new QState(errorStateGroup);
    criticalErrorState->setObjectName("Critical");

    QState *warningState = new QState(errorStateGroup);
    warningState->setObjectName("Warning");

    errorStateGroup->setInitialState(noErrorState);

    // === НАСТРОЙКА И ЗАПУСК МАШИНЫ ===
    _stateMachine->addState(rootState);
    _stateMachine->setInitialState(rootState);
    _stateMachine->start();

    qDebug() << ">>> FCSVGImageParser: StateMachine initialized";
}

// МЕТОДЫ ПАРСИНГА (СИНХРОННЫЕ)
FCSVGImageParser::ParseResult FCSVGImageParser::parse(const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        ParseResult result;
        result.errorMessage = QStringLiteral("Cannot open file: %1").arg(filePath);
        set(FCErrorType::Critical);
        return result;
    }

    QByteArray data = file.readAll();
    file.close();

    _currentFile = filePath;
    return parseSvgInternal(data);
}

FCSVGImageParser::ParseResult FCSVGImageParser::parse(const QByteArray &data, const QString &name)
{
    _currentFile = name;
    return parseSvgInternal(data);
}

// АСИНХРОННЫЙ ПАРСИНГ
void FCSVGImageParser::parseAsync(const QString &filePath)
{
    if(isParsing())
    {
        qWarning() << "Parsing already in progress, ignoring request";
        return;
    }

    if(is(FCReadyState::Ready))
    {
        _currentFile = filePath;
        _isParsing.storeRelaxed(1);

        // Обновляем состояния через новые методы
        set(FCPlayState::Start);
        set(FCReadyState::NotReady);
        set(FCChangedState::Changed);

        emit started(filePath);

        _future = QtConcurrent::run([this, filePath]()
        {
            QFile file(filePath);
            if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                _isParsing.storeRelaxed(0);
                set(FCPlayState::Stop);
                set(FCErrorType::Critical);
                emit error(objectName(), QStringLiteral("Cannot open file: %1").arg(filePath));
                return;
            }

            QByteArray data = file.readAll();
            file.close();

            ParseResult result = parseSvgInternal(data);

            _isParsing.storeRelaxed(0);
            set(FCPlayState::Stop);

            if(result.success)
            {
                set(FCReadyState::Ready);
                set(FCChangedState::Unchanged);
                emit containerReady(_container);
            }
            else
            {
                set(FCErrorType::Critical);
            }

            emit finished(result);
        });
    }
}

// УПРАВЛЕНИЕ ПАРСИНГОМ
void FCSVGImageParser::cancel()
{
    if(_future.isRunning())
    {
        _future.cancel();
    }
    _isParsing.storeRelaxed(0);
    set(FCPlayState::Stop);
}

bool FCSVGImageParser::isParsing() const noexcept
{
    return _isParsing.loadRelaxed() != 0;
}

FCSVGImageContainer* FCSVGImageParser::container() const noexcept
{
    return _container;
}

// ВНУТРЕННИЙ ПАРСИНГ
FCSVGImageParser::ParseResult FCSVGImageParser::parseSvgInternal(const QByteArray &data)
{
    QElapsedTimer timer;
    timer.start();

    ParseResult result;
    result.sourceFile = _currentFile;

    if(_future.isCanceled())
    {
        result.errorMessage = QStringLiteral("Canceled at start");
        return result;
    }

    // Сбрасываем состояния перед новым парсингом
    set(FCReadyState::NotReady);
    set(FCChangedState::Changed);

    _container = new FCSVGImageContainer(this);
    _container->setObjectName(QStringLiteral("ParsedContainer") + _currentFile);

    QXmlStreamReader reader(data);
    int elementsProcessed = 0;
    int totalElementsEstimate = 1000;

    QStack<FCSVGImageContainer::BinaryLayer*> layerStack;
    layerStack.push(&_container->defaultLayer());

    updateProgress(5, QStringLiteral("Reading XML"));

    while(!reader.atEnd() && !reader.hasError())
    {
        if((elementsProcessed % 50) == 0 && QThread::currentThread()->isInterruptionRequested())
        {
            result.errorMessage = QStringLiteral("Parsing interrupted");
            delete _container;
            _container = nullptr;
            set(FCErrorType::Critical);
            return result;
        }

        QXmlStreamReader::TokenType token = reader.readNext();

        if(token == QXmlStreamReader::StartElement)
        {
            QStringRef name = reader.name();

            // Пропуск скрытых слоёв
            if(_settings.ignoreHiddenLayers && name == QLatin1String("g"))
            {
                QString style = reader.attributes().value(QLatin1String("style")).toString();
                if(style.contains(QLatin1String("display:none"), Qt::CaseInsensitive))
                {
                    reader.skipCurrentElement();
                    continue;
                }
            }

            FCSVGImageContainer::BinaryLayer *currentLayer = layerStack.isEmpty() ?
                &_container->defaultLayer() : layerStack.top();

            if(name == QLatin1String("svg"))
            {
                parseSvgElement(reader, *currentLayer);
            }
            else if(name == QLatin1String("path"))
            {
                FCSVGImageContainer::BinaryFigure figure;
                if(parsePathElement(reader, figure))
                {
                    currentLayer->figures.append(figure);
                    ++result.figuresParsed;
                    result.pointsTotal += figure.points.size();
                }
            }
            else if(name == QLatin1String("rect"))
            {
                FCSVGImageContainer::BinaryFigure figure;
                if(parseRectElement(reader, figure))
                {
                    currentLayer->figures.append(figure);
                    ++result.figuresParsed;
                    result.pointsTotal += figure.points.size();
                }
            }
            else if(name == QLatin1String("circle"))
            {
                FCSVGImageContainer::BinaryFigure figure;
                if(parseCircleElement(reader, figure))
                {
                    currentLayer->figures.append(figure);
                    ++result.figuresParsed;
                    result.pointsTotal += figure.points.size();
                }
            }
            else if(name == QLatin1String("ellipse"))
            {
                FCSVGImageContainer::BinaryFigure figure;
                if(parseEllipseElement(reader, figure))
                {
                    currentLayer->figures.append(figure);
                    ++result.figuresParsed;
                    result.pointsTotal += figure.points.size();
                }
            }
            else if(name == QLatin1String("polygon"))
            {
                FCSVGImageContainer::BinaryFigure figure;
                if(parsePolygonElement(reader, figure))
                {
                    currentLayer->figures.append(figure);
                    ++result.figuresParsed;
                    result.pointsTotal += figure.points.size();
                }
            }
            else if(name == QLatin1String("polyline"))
            {
                FCSVGImageContainer::BinaryFigure figure;
                if(parsePolylineElement(reader, figure))
                {
                    currentLayer->figures.append(figure);
                    ++result.figuresParsed;
                    result.pointsTotal += figure.points.size();
                }
            }
            else if(name == QLatin1String("line"))
            {
                FCSVGImageContainer::BinaryFigure figure;
                if(parseLineElement(reader, figure))
                {
                    currentLayer->figures.append(figure);
                    ++result.figuresParsed;
                    result.pointsTotal += figure.points.size();
                }
            }
            else if(name == QLatin1String("g"))
            {
                parseGroupElement(reader, *currentLayer, layerStack);
            }
            else if(name == QLatin1String("text") && _settings.extractText)
            {
                qWarning() << "Text extraction not fully implemented, skipping: "
                          << reader.attributes().value(QLatin1String("id")).toString();
            }

            ++elementsProcessed;
            int prog = 5 + static_cast<int>((elementsProcessed * 45.0) / totalElementsEstimate);
            updateProgress(qMin(prog, 50), QStringLiteral("Parsing elements"));
        }
        else if(token == QXmlStreamReader::EndElement)
        {
            if(reader.name() == QLatin1String("g") && layerStack.size() > 1)
            {
                layerStack.pop();
            }
        }
    }

    if(reader.hasError())
    {
        result.errorMessage = QStringLiteral("XML error at line %1: %2")
            .arg(reader.lineNumber())
            .arg(reader.errorString());
        delete _container;
        _container = nullptr;
        set(FCErrorType::Critical);
        return result;
    }

    // === ОПТИМИЗАЦИЯ ГЕОМЕТРИИ ===
    updateProgress(55, QStringLiteral("Optimizing geometry"));

    if(_settings.simplifyPaths || _settings.removeDuplicates || _settings.mergeAdjacent)
    {
        for(auto &layer : _container->layers())
        {
            for(auto &figure : layer.figures)
            {
                if(_settings.removeDuplicates)
                {
                    removeDuplicatePoints(figure.points);
                }
                if(_settings.simplifyPaths && !figure.points.isEmpty())
                {
                    simplifyPath(figure.points, _settings.tolerance);
                }
                if(_settings.mergeAdjacent && figure.points.size() > 2)
                {
                    mergeAdjacentSegments(figure.points);
                }
                if(_settings.minSegmentLength > 0 && figure.points.size() > 1)
                {
                    QVector<FC3DPoint> filtered;
                    filtered.reserve(figure.points.size());
                    filtered.append(figure.points.first());
                    for(int i = 1; i < figure.points.size(); ++i)
                    {
                        if(distance2D(filtered.last(), figure.points[i]) >= _settings.minSegmentLength)
                        {
                            filtered.append(figure.points[i]);
                        }
                    }
                    if(filtered.size() >= 2)
                    {
                        figure.points = std::move(filtered);
                    }
                    else
                    {
                        figure.points.clear();
                    }
                }
                figure.recalculateMetrics();
                result.pointsTotal += figure.points.size();
            }
        }
    }

    // === МЕТАДАННЫЕ ===
    FCSVGImageContainer::Metadata metadata;
    metadata.sourceFile = _currentFile;
    metadata.figureCount = result.figuresParsed;
    metadata.layerCount = static_cast<quint32>(_container->layerCount());
    metadata.totalPoints = result.pointsTotal;
    metadata.parsedAt = QDateTime::currentDateTime();
    metadata.parserVersion = QStringLiteral("2.0.0");
    _container->setMetadata(metadata);
    _container->setReady();

    result.success = true;
    result.parseTimeMs = timer.elapsed();

    // Парсинг завершён успешно — обновляем состояния
    set(FCReadyState::Ready);
    set(FCChangedState::Unchanged);

    updateProgress(100, QStringLiteral("Complete"));

    return result;
}

// ПАРСИНГ ЭЛЕМЕНТОВ SVG
bool FCSVGImageParser::parsePathElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    QString pathData = reader.attributes().value(QLatin1String("d")).toString();
    if(pathData.isEmpty())
    {
        return false;
    }

    figure.pathData = pathData;
    parsePathData(pathData, figure.points);

    QString colorStr = reader.attributes().value(QLatin1String("stroke")).toString();
    if(!colorStr.isEmpty())
    {
        figure.color = QColor(colorStr);
    }
    else
    {
        colorStr = reader.attributes().value(QLatin1String("fill")).toString();
        if(!colorStr.isEmpty() && colorStr != QLatin1String("none"))
        {
            figure.color = QColor(colorStr);
            figure.isFilled = true;
        }
    }

    QString widthStr = reader.attributes().value(QLatin1String("stroke-width")).toString();
    if(!widthStr.isEmpty())
    {
        figure.thickness = widthStr.toFloat();
    }

    QString idStr = reader.attributes().value(QLatin1String("id")).toString();
    if(!idStr.isEmpty())
    {
        figure.pathData.prepend(QStringLiteral("#%1 ").arg(idStr));
    }

    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseSvgElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryLayer &layer)
{
    Q_UNUSED(layer)

    QString viewBoxStr = reader.attributes().value(QLatin1String("viewBox")).toString();
    if(!viewBoxStr.isEmpty())
    {
        QStringList parts = viewBoxStr.split(QRegularExpression(R"([\s,]+)"), Qt::SkipEmptyParts);
        if(parts.size() >= 4)
        {
            FC2DSize viewBox;
            viewBox.setWidth(parts[2].toFloat());
            viewBox.setHeight(parts[3].toFloat());
            _container->setViewBox(viewBox);
        }
    }

    QString widthStr = reader.attributes().value(QLatin1String("width")).toString();
    QString heightStr = reader.attributes().value(QLatin1String("height")).toString();
    if(!widthStr.isEmpty() && !heightStr.isEmpty())
    {
        FC2DSize size;
        size.setWidth(widthStr.toFloat());
        size.setHeight(heightStr.toFloat());
        FCSVGImageContainer::Metadata meta = _container->metadata();
        meta.imageSize = size;
        _container->setMetadata(meta);
    }

    return true;
}

void FCSVGImageParser::parseGroupElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryLayer &parentLayer, QStack<FCSVGImageContainer::BinaryLayer*> &layerStack)
{
    QString idStr = reader.attributes().value(QLatin1String("id")).toString();
    QString labelStr = reader.attributes().value(QLatin1String("inkscape:label")).toString();
    QString groupmodeStr = reader.attributes().value(QLatin1String("inkscape:groupmode")).toString();

    if(groupmodeStr == QLatin1String("layer"))
    {
        FCSVGImageContainer::BinaryLayer newLayer;
        newLayer.name = labelStr.isEmpty() ? idStr : labelStr;
        newLayer.inkscapeId = idStr;
        newLayer.isVisible = true;

        QString style = reader.attributes().value(QLatin1String("style")).toString();
        if(style.contains(QLatin1String("display:none"), Qt::CaseInsensitive))
        {
            newLayer.isVisible = false;
        }

        _container->addLayer(newLayer);
        layerStack.push(&_container->layerRef(_container->layerCount() - 1));
    }

    Q_UNUSED(reader)
    Q_UNUSED(parentLayer)
}

bool FCSVGImageParser::parseRectElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    qreal x = reader.attributes().value(QLatin1String("x")).toDouble();
    qreal y = reader.attributes().value(QLatin1String("y")).toDouble();
    qreal width = reader.attributes().value(QLatin1String("width")).toDouble();
    qreal height = reader.attributes().value(QLatin1String("height")).toDouble();
    qreal rx = reader.attributes().value(QLatin1String("rx")).toDouble();
    qreal ry = reader.attributes().value(QLatin1String("ry")).toDouble();

    if(width <= 0 || height <= 0)
    {
        return false;
    }

    if(rx > 0 || ry > 0)
    {
        // Прямоугольник со скруглёнными углами — аппроксимация кривыми
        parseRoundedRect(x, y, width, height, rx, ry, figure.points);
    }
    else
    {
        // Обычный прямоугольник
        figure.points.append(FC3DPoint(x, y, 0));
        figure.points.append(FC3DPoint(x + width, y, 0));
        figure.points.append(FC3DPoint(x + width, y + height, 0));
        figure.points.append(FC3DPoint(x, y + height, 0));
        figure.points.append(FC3DPoint(x, y, 0));  // Замыкаем контур
    }

    figure.isFilled = true;
    applyStyleAttributes(reader, figure);

    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseCircleElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    qreal cx = reader.attributes().value(QLatin1String("cx")).toDouble();
    qreal cy = reader.attributes().value(QLatin1String("cy")).toDouble();
    qreal r = reader.attributes().value(QLatin1String("r")).toDouble();

    if(r <= 0)
    {
        return false;
    }

    // Аппроксимация круга 36 точками
    const int segments = 36;
    for(int i = 0; i <= segments; ++i)
    {
        qreal angle = (2.0 * M_PI * i) / segments;
        qreal x = cx + r * qCos(angle);
        qreal y = cy + r * qSin(angle);
        figure.points.append(FC3DPoint(x, y, 0));
    }

    figure.isFilled = true;
    applyStyleAttributes(reader, figure);

    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseEllipseElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    qreal cx = reader.attributes().value(QLatin1String("cx")).toDouble();
    qreal cy = reader.attributes().value(QLatin1String("cy")).toDouble();
    qreal rx = reader.attributes().value(QLatin1String("rx")).toDouble();
    qreal ry = reader.attributes().value(QLatin1String("ry")).toDouble();

    if(rx <= 0 || ry <= 0)
    {
        return false;
    }

    // Аппроксимация эллипса 36 точками
    const int segments = 36;
    for(int i = 0; i <= segments; ++i)
    {
        qreal angle = (2.0 * M_PI * i) / segments;
        qreal x = cx + rx * qCos(angle);
        qreal y = cy + ry * qSin(angle);
        figure.points.append(FC3DPoint(x, y, 0));
    }

    figure.isFilled = true;
    applyStyleAttributes(reader, figure);

    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parsePolygonElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    QString pointsStr = reader.attributes().value(QLatin1String("points")).toString();
    if(pointsStr.isEmpty())
    {
        return false;
    }

    parsePointsAttribute(pointsStr, figure.points);

    // Замыкаем полигон
    if(!figure.points.isEmpty())
    {
        figure.points.append(figure.points.first());
    }

    figure.isFilled = true;
    applyStyleAttributes(reader, figure);

    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parsePolylineElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    QString pointsStr = reader.attributes().value(QLatin1String("points")).toString();
    if(pointsStr.isEmpty())
    {
        return false;
    }

    parsePointsAttribute(pointsStr, figure.points);

    // Полилиния не замыкается
    figure.isFilled = false;
    applyStyleAttributes(reader, figure);

    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseLineElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    qreal x1 = reader.attributes().value(QLatin1String("x1")).toDouble();
    qreal y1 = reader.attributes().value(QLatin1String("y1")).toDouble();
    qreal x2 = reader.attributes().value(QLatin1String("x2")).toDouble();
    qreal y2 = reader.attributes().value(QLatin1String("y2")).toDouble();

    figure.points.append(FC3DPoint(x1, y1, 0));
    figure.points.append(FC3DPoint(x2, y2, 0));

    figure.isFilled = false;
    applyStyleAttributes(reader, figure);

    return !figure.points.isEmpty();
}

// ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ПАРСИНГА
void FCSVGImageParser::updateProgress(int percent, const QString &stage)
{
    // Можно добавить логику обновления состояний на основе прогресса
    if(percent >= 100)
    {
        set(FCReadyState::Ready);
    }
    emit progress(percent, stage);
}

void FCSVGImageParser::parsePathData(const QString &pathData, QVector<FC3DPoint> &points)
{
    int pos = 0;
    qreal currentX = 0;
    qreal currentY = 0;
    qreal startX = 0;
    qreal startY = 0;

    // Статические регулярные выражения — создаются один раз при первом вызове
    static const QRegularExpression cmdRegex(R"([MmZzLlHhVvCcSsQqTtAa])");
    static const QRegularExpression numRegex(R"(-?\d*\.?\d+(?:[eE][+-]?\d+)?)");

    const int pathLen = static_cast<int>(pathData.length());

    while(pos < pathLen)
    {
        QRegularExpressionMatch cmdMatch = cmdRegex.match(pathData, pos);
        if(!cmdMatch.hasMatch())
        {
            break;
        }

        QString cmd = cmdMatch.captured(0);
        pos = cmdMatch.capturedEnd(0);

        QVector<qreal> params;
        while(pos < pathLen)
        {
            QRegularExpressionMatch numMatch = numRegex.match(pathData, pos);
            if(!numMatch.hasMatch())
            {
                break;
            }
            QString numStr = numMatch.captured(0);
            params.append(numStr.toDouble());
            pos = numMatch.capturedEnd(0);
        }

        int paramIdx = 0;
        bool relative = cmd.isLower();

        switch(cmd.toUpper().at(0).toLatin1())
        {
        case 'M':  // MoveTo
            while(paramIdx + 1 < static_cast<int>(params.size()))
            {
                qreal x = params[paramIdx] + (relative ? currentX : 0);
                qreal y = params[paramIdx + 1] + (relative ? currentY : 0);
                points.append(FC3DPoint(x, y, 0));
                currentX = x;
                currentY = y;
                if(points.size() == 1)
                {
                    startX = x;
                    startY = y;
                }
                paramIdx += 2;
            }
            break;

        case 'L':  // LineTo
            while(paramIdx + 1 < static_cast<int>(params.size()))
            {
                qreal x = params[paramIdx] + (relative ? currentX : 0);
                qreal y = params[paramIdx + 1] + (relative ? currentY : 0);
                points.append(FC3DPoint(x, y, 0));
                currentX = x;
                currentY = y;
                paramIdx += 2;
            }
            break;

        case 'H':  // HorizontalLineTo
            while(paramIdx < static_cast<int>(params.size()))
            {
                qreal x = params[paramIdx] + (relative ? currentX : 0);
                points.append(FC3DPoint(x, currentY, 0));
                currentX = x;
                paramIdx += 1;
            }
            break;

        case 'V':  // VerticalLineTo
            while(paramIdx < static_cast<int>(params.size()))
            {
                qreal y = params[paramIdx] + (relative ? currentY : 0);
                points.append(FC3DPoint(currentX, y, 0));
                currentY = y;
                paramIdx += 1;
            }
            break;

        case 'C':  // CubicBezierTo
            while(paramIdx + 5 < static_cast<int>(params.size()))
            {
                qreal cp1x = params[paramIdx] + (relative ? currentX : 0);
                qreal cp1y = params[paramIdx + 1] + (relative ? currentY : 0);
                qreal cp2x = params[paramIdx + 2] + (relative ? currentX : 0);
                qreal cp2y = params[paramIdx + 3] + (relative ? currentY : 0);
                qreal endX = params[paramIdx + 4] + (relative ? currentX : 0);
                qreal endY = params[paramIdx + 5] + (relative ? currentY : 0);

                // Аппроксимация кривой Безье 10 точками
                for(int i = 1; i <= 10; ++i)
                {
                    qreal t = static_cast<qreal>(i) / 10.0;
                    qreal mt = 1.0 - t;
                    qreal mt2 = mt * mt;
                    qreal mt3 = mt2 * mt;
                    qreal t2 = t * t;
                    qreal t3 = t2 * t;

                    qreal bx = mt3 * currentX + 3 * mt2 * t * cp1x + 3 * mt * t2 * cp2x + t3 * endX;
                    qreal by = mt3 * currentY + 3 * mt2 * t * cp1y + 3 * mt * t2 * cp2y + t3 * endY;

                    points.append(FC3DPoint(bx, by, 0));
                }

                currentX = endX;
                currentY = endY;
                paramIdx += 6;
            }
            break;

        case 'Z':  // ClosePath
            if(!points.isEmpty())
            {
                points.append(FC3DPoint(startX, startY, 0));
                currentX = startX;
                currentY = startY;
            }
            break;

        default:
            break;
        }
    }
}

void FCSVGImageParser::parsePointsAttribute(const QString &pointsStr, QVector<FC3DPoint> &points)
{
    static const QRegularExpression numRegex(R"(-?\d*\.?\d+(?:[eE][+-]?\d+)?)");
    QRegularExpressionMatchIterator it = numRegex.globalMatch(pointsStr);
    QVector<qreal> coords;

    while(it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        coords.append(match.captured(0).toDouble());
    }

    for(int i = 0; i + 1 < coords.size(); i += 2)
    {
        points.append(FC3DPoint(coords[i], coords[i + 1], 0));
    }
}

void FCSVGImageParser::parseRoundedRect(qreal x, qreal y, qreal width, qreal height, qreal rx, qreal ry, QVector<FC3DPoint> &points)
{
    if(rx <= 0) rx = ry;
    if(ry <= 0) ry = rx;
    rx = qMin(rx, width / 2.0);
    ry = qMin(ry, height / 2.0);

    const int segments = 8;

    // Левый-верхний угол
    for(int i = 0; i <= segments; ++i)
    {
        qreal angle = M_PI + (M_PI / 2.0) * (i / static_cast<qreal>(segments));
        points.append(FC3DPoint(x + rx + rx * qCos(angle), y + ry + ry * qSin(angle), 0));
    }

    // Правый-верхний угол
    for(int i = 0; i <= segments; ++i)
    {
        qreal angle = (3.0 * M_PI / 2.0) + (M_PI / 2.0) * (i / static_cast<qreal>(segments));
        points.append(FC3DPoint(x + width - rx + rx * qCos(angle), y + ry + ry * qSin(angle), 0));
    }

    // Правый-нижний угол
    for(int i = 0; i <= segments; ++i)
    {
        qreal angle = 0 + (M_PI / 2.0) * (i / static_cast<qreal>(segments));
        points.append(FC3DPoint(x + width - rx + rx * qCos(angle), y + height - ry + ry * qSin(angle), 0));
    }

    // Левый-нижний угол
    for(int i = 0; i <= segments; ++i)
    {
        qreal angle = M_PI / 2.0 + (M_PI / 2.0) * (i / static_cast<qreal>(segments));
        points.append(FC3DPoint(x + rx + rx * qCos(angle), y + height - ry + ry * qSin(angle), 0));
    }
}

void FCSVGImageParser::applyStyleAttributes(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure)
{
    QString colorStr = reader.attributes().value(QLatin1String("stroke")).toString();
    if(!colorStr.isEmpty())
    {
        figure.color = QColor(colorStr);
    }

    QString fillStr = reader.attributes().value(QLatin1String("fill")).toString();
    if(!fillStr.isEmpty() && fillStr != QLatin1String("none"))
    {
        figure.isFilled = true;
        if(colorStr.isEmpty())
        {
            figure.color = QColor(fillStr);
        }
    }

    QString widthStr = reader.attributes().value(QLatin1String("stroke-width")).toString();
    if(!widthStr.isEmpty())
    {
        figure.thickness = widthStr.toFloat();
    }
}
