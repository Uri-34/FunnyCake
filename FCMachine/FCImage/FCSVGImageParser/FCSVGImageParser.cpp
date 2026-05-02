// FCSVGImageParser.cpp
// Реализация парсера SVG-файлов в бинарный формат для генерации G-кода
// Версия: 2.1.0 (Clean State Machine Free)
// Архитектура: Управление состояниями через FCParserState (QStateMachine УДАЛЁН)

#include "FCSVGImageParser.h"
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
#include <QDebug>
#include <algorithm>

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ОПТИМИЗАЦИИ ГЕОМЕТРИИ
// ============================================================================

void FCSVGImageParser::removeDuplicatePoints(QVector<FC2DPoint> &points) noexcept
{
    if (points.size() < 2) {
        return;
    }
    auto last = std::unique(points.begin(), points.end(),
        [](const FC2DPoint &a, const FC2DPoint &b) {
            return qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y());
        });
    points.erase(last, points.end());
}

void FCSVGImageParser::removeDuplicatePoints(QVector<FC3DPoint> &points) noexcept
{
    if (points.size() < 2) {
        return;
    }
    auto last = std::unique(points.begin(), points.end(),
        [](const FC3DPoint &a, const FC3DPoint &b) {
            return qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y());
        });
    points.erase(last, points.end());
}

qreal FCSVGImageParser::distance2D(const FC3DPoint &a, const FC3DPoint &b) noexcept
{
    qreal dx = b.x() - a.x();
    qreal dy = b.y() - a.y();
    return qSqrt(dx * dx + dy * dy);
}

void FCSVGImageParser::simplifyPath(QVector<FC3DPoint> &points, qreal tolerance) noexcept
{
    if (points.size() <= 2) {
        return;
    }

    QVector<int> keep;
    keep.reserve(points.size());
    keep.append(0);
    keep.append(points.size() - 1);

    QVector<QPair<int, int>> stack;
    stack.reserve(points.size() / 2);
    stack.append(qMakePair(0, points.size() - 1));

    while (!stack.isEmpty()) {
        auto [first, last] = stack.takeLast();
        if (last - first <= 1) {
            continue;
        }

        qreal maxDist = 0;
        int maxIdx = first;
        qreal p1x = points[first].x();
        qreal p1y = points[first].y();
        qreal p2x = points[last].x();
        qreal p2y = points[last].y();
        qreal dx = p2x - p1x;
        qreal dy = p2y - p1y;
        qreal lenSq = dx * dx + dy * dy;

        for (int i = first + 1; i < last; ++i) {
            qreal dist = 0;
            if (lenSq == 0) {
                dist = distance2D(points[i], points[first]);
            } else {
                qreal t = qBound(qreal(0),
                    ((points[i].x() - p1x) * dx + (points[i].y() - p1y) * dy) / lenSq,
                    qreal(1));
                dist = distance2D(points[i], { p1x + t * dx, p1y + t * dy, 0 });
            }
            if (dist > maxDist) {
                maxDist = dist;
                maxIdx = i;
            }
        }

        if (maxDist > tolerance) {
            keep.append(maxIdx);
            stack.append(qMakePair(first, maxIdx));
            stack.append(qMakePair(maxIdx, last));
        }
    }

    std::sort(keep.begin(), keep.end());

    QVector<FC3DPoint> result;
    result.reserve(keep.size());
    for (int idx : keep) {
        result.append(points[idx]);
    }
    points = std::move(result);
}

void FCSVGImageParser::mergeAdjacentSegments(QVector<FC3DPoint> &points) noexcept
{
    if (points.size() < 3) {
        return;
    }

    QVector<FC3DPoint> result;
    result.reserve(points.size());
    result.append(points.first());

    for (int i = 1; i < points.size() - 1; ++i) {
        qreal dx1 = points[i].x() - points[i - 1].x();
        qreal dy1 = points[i].y() - points[i - 1].y();
        qreal dx2 = points[i + 1].x() - points[i].x();
        qreal dy2 = points[i + 1].y() - points[i].y();
        if (qAbs(dx1 * dy2 - dy1 * dx2) > 0.001) {
            result.append(points[i]);
        }
    }

    if (!points.isEmpty()) {
        result.append(points.last());
    }

    if (result.size() >= 2) {
        points = std::move(result);
    }
}

// ============================================================================
// КОНСТРУКТОРЫ / ДЕСТРУКТОР
// ============================================================================

FCSVGImageParser::FCSVGImageParser(QObject *parent)
    : QObject(parent)
    , _container(nullptr)
    , _isParsing{0}
    , _state(FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Unchanged, FCErrorType::None)
{
    setObjectName("SVGParser");
    _transformStack.push(QTransform());
}

FCSVGImageParser::FCSVGImageParser(const ParserSettings &settings, QObject *parent)
    : QObject(parent)
    , _settings(settings)
    , _container(nullptr)
    , _isParsing{0}
    , _state(FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Unchanged, FCErrorType::None)
{
    setObjectName("SVGParser");
    _transformStack.push(QTransform());
}

FCSVGImageParser::~FCSVGImageParser()
{
    cancel();
    if (_container) {
        delete _container;
        _container = nullptr;
    }
}

// ============================================================================
// УПРАВЛЕНИЕ СОСТОЯНИЯМИ
// ============================================================================

inline void FCSVGImageParser::set(FCReadyState state)
{
    if (!_state.is(state)) {
        _state.set(state);
        emit readyStateChanged(state);
    }
}

inline void FCSVGImageParser::set(FCPlayState state)
{
    if (!_state.is(state)) {
        _state.set(state);
        emit playStateChanged(state);
    }
}

inline void FCSVGImageParser::set(FCChangedState state)
{
    if (!_state.is(state)) {
        _state.set(state);
        emit changedStateChanged(state);
    }
}

inline void FCSVGImageParser::set(FCErrorType type)
{
    if (!_state.is(type)) {
        _state.set(type);
        emit errorTypeChanged(type);
    }
}

void FCSVGImageParser::setSettings(const ParserSettings &settings)
{
    _settings = settings;
}

FCSVGImageParser::ParserSettings FCSVGImageParser::settings() const noexcept
{
    return _settings;
}

// ============================================================================
// ПАРСИНГ (СИНХРОННЫЙ / АСИНХРОННЫЙ)
// ============================================================================

FCSVGImageParser::ParseResult FCSVGImageParser::parse(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ParseResult res;
        res.errorMessage = QStringLiteral("Cannot open file: %1").arg(filePath);
        set(FCErrorType::Parse);
        return res;
    }
    _currentFile = filePath;
    return parseSvgInternal(file.readAll());
}

FCSVGImageParser::ParseResult FCSVGImageParser::parse(const QByteArray &data, const QString &name)
{
    _currentFile = name;
    return parseSvgInternal(data);
}

void FCSVGImageParser::parseAsync(const QString &filePath)
{
    if (isParsing()) {
        qWarning() << "Parsing already in progress";
        return;
    }

    _currentFile = filePath;
    _isParsing.storeRelaxed(1);
    set(FCPlayState::Start);
    set(FCReadyState::NotReady);
    set(FCChangedState::Changed);
    emit started(filePath);

    _future = QtConcurrent::run([this, filePath]() {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            _isParsing.storeRelaxed(0);
            set(FCPlayState::Stop);
            set(FCErrorType::Parse);
            emit error(objectName(), QStringLiteral("Cannot open file: %1").arg(filePath));
            return;
        }

        ParseResult res = parseSvgInternal(file.readAll());

        _isParsing.storeRelaxed(0);
        set(FCPlayState::Stop);

        if (res.success) {
            set(FCReadyState::Ready);
            set(FCChangedState::Unchanged);
            emit containerReady(_container);
        } else {
            set(FCErrorType::Parse);
        }
        emit finished(res);
    });
}

void FCSVGImageParser::cancel()
{
    if (_future.isRunning()) {
        _future.cancel();
    }
    _isParsing.storeRelaxed(0);
    set(FCPlayState::Stop);
}

bool FCSVGImageParser::isParsing() const noexcept
{
    return _isParsing.loadRelaxed() != 0;
}

FCImageBinaryContainer *FCSVGImageParser::container() const noexcept
{
    return _container;
}

// ============================================================================
// ВНУТРЕННИЙ ПАРСИНГ С ПРОГРЕССОМ И ТРАНСФОРМАЦИЯМИ
// ============================================================================

FCSVGImageParser::ParseResult FCSVGImageParser::parseSvgInternal(const QByteArray &data)
{
    QElapsedTimer timer;
    timer.start();

    ParseResult result;
    result.sourceFile = _currentFile;

    if (_future.isCanceled()) {
        result.errorMessage = "Canceled at start";
        return result;
    }

    set(FCReadyState::NotReady);
    set(FCChangedState::Changed);

    delete _container;
    _container = new FCImageBinaryContainer(this);
    _container->setObjectName("ParsedContainer_" + _currentFile);
    _transformStack.clear();
    _transformStack.push(QTransform());

    // Динамическая оценка для точного 1% прогресса
    int totalEstimate = 0;
    for (char c : data) {
        if (c == '<') {
            ++totalEstimate;
        }
    }
    if (totalEstimate == 0) {
        totalEstimate = 1;
    }

    int lastPercent = 0;
    QXmlStreamReader reader(data);
    int elementsProcessed = 0;
    QStack<FCImageBinaryContainer::BinaryLayer *> layerStack;
    layerStack.push(&_container->defaultLayer());

    updateProgress(5, "Reading XML");

    while (!reader.atEnd() && !reader.hasError()) {
        if ((elementsProcessed % 10 == 0) && _future.isCanceled()) {
            result.errorMessage = "Parsing cancelled by user";
            delete _container;
            _container = nullptr;
            set(FCErrorType::Parse);
            return result;
        }

        reader.readNext();

        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            ++elementsProcessed;

            int currentPercent = qMin(99, (elementsProcessed * 100) / totalEstimate);
            if (currentPercent > lastPercent) {
                emit progress(currentPercent, "Parsing elements");
                lastPercent = currentPercent;
            }

            QStringView name = reader.name();

            if (_settings.ignoreHiddenLayers && name == QLatin1String("g")) {
                QString style = reader.attributes().value(QLatin1String("style")).toString();
                if (style.contains(QLatin1String("display:none"), Qt::CaseInsensitive)) {
                    reader.skipCurrentElement();
                    continue;
                }
            }

            FCImageBinaryContainer::BinaryLayer *curLayer =
                layerStack.isEmpty() ? &_container->defaultLayer() : layerStack.top();

            if (name == QLatin1String("svg")) {
                parseSvgElement(reader, *curLayer);
            } else if (name == QLatin1String("path")) {
                FCImageBinaryContainer::BinaryFigure fig;
                if (parsePathElement(reader, fig)) {
                    curLayer->figures.append(fig);
                    ++result.figuresParsed;
                    result.pointsTotal += fig.points.size();
                }
            } else if (name == QLatin1String("rect")) {
                FCImageBinaryContainer::BinaryFigure fig;
                if (parseRectElement(reader, fig)) {
                    curLayer->figures.append(fig);
                    ++result.figuresParsed;
                    result.pointsTotal += fig.points.size();
                }
            } else if (name == QLatin1String("circle")) {
                FCImageBinaryContainer::BinaryFigure fig;
                if (parseCircleElement(reader, fig)) {
                    curLayer->figures.append(fig);
                    ++result.figuresParsed;
                    result.pointsTotal += fig.points.size();
                }
            } else if (name == QLatin1String("ellipse")) {
                FCImageBinaryContainer::BinaryFigure fig;
                if (parseEllipseElement(reader, fig)) {
                    curLayer->figures.append(fig);
                    ++result.figuresParsed;
                    result.pointsTotal += fig.points.size();
                }
            } else if (name == QLatin1String("polygon")) {
                FCImageBinaryContainer::BinaryFigure fig;
                if (parsePolygonElement(reader, fig)) {
                    curLayer->figures.append(fig);
                    ++result.figuresParsed;
                    result.pointsTotal += fig.points.size();
                }
            } else if (name == QLatin1String("polyline")) {
                FCImageBinaryContainer::BinaryFigure fig;
                if (parsePolylineElement(reader, fig)) {
                    curLayer->figures.append(fig);
                    ++result.figuresParsed;
                    result.pointsTotal += fig.points.size();
                }
            } else if (name == QLatin1String("line")) {
                FCImageBinaryContainer::BinaryFigure fig;
                if (parseLineElement(reader, fig)) {
                    curLayer->figures.append(fig);
                    ++result.figuresParsed;
                    result.pointsTotal += fig.points.size();
                }
            } else if (name == QLatin1String("g")) {
                pushTransform(reader);
                parseGroupElement(reader, *curLayer, layerStack);
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement &&
                   reader.name() == QLatin1String("g")) {
            popTransform();
            if (layerStack.size() > 1) {
                layerStack.pop();
            }
        }
    }

    if (reader.hasError()) {
        result.errorMessage = QStringLiteral("XML error at line %1: %2")
                                  .arg(reader.lineNumber())
                                  .arg(reader.errorString());
        delete _container;
        _container = nullptr;
        set(FCErrorType::Parse);
        return result;
    }

    updateProgress(55, "Optimizing geometry");

    if (_settings.simplifyPaths || _settings.removeDuplicates || _settings.mergeAdjacent) {
        for (auto &layer : _container->layers()) {
            for (auto &fig : layer.figures) {
                if (_settings.removeDuplicates) {
                    removeDuplicatePoints(fig.points);
                }
                if (_settings.simplifyPaths && !fig.points.isEmpty()) {
                    simplifyPath(fig.points, _settings.tolerance);
                }
                if (_settings.mergeAdjacent && fig.points.size() > 2) {
                    mergeAdjacentSegments(fig.points);
                }
                if (_settings.minSegmentLength > 0 && fig.points.size() > 1) {
                    QVector<FC3DPoint> filtered;
                    filtered.reserve(fig.points.size());
                    filtered.append(fig.points.first());
                    for (int i = 1; i < fig.points.size(); ++i) {
                        if (distance2D(filtered.last(), fig.points[i]) >= _settings.minSegmentLength) {
                            filtered.append(fig.points[i]);
                        }
                    }
                    fig.points = filtered.size() >= 2 ? std::move(filtered) : QVector<FC3DPoint>();
                }
                fig.recalculateMetrics();
                result.pointsTotal += fig.points.size();
            }
        }
    }

    FCImageBinaryContainer::Metadata meta;
    meta.sourceFile = _currentFile;
    meta.figureCount = result.figuresParsed;
    meta.layerCount = static_cast<quint32>(_container->layerCount());
    meta.totalPoints = result.pointsTotal;
    meta.parsedAt = QDateTime::currentDateTime();
    meta.parserVersion = "2.1.0";
    _container->setMetadata(meta);
    _container->setReady();

    result.success = true;
    result.parseTimeMs = timer.elapsed();

    set(FCReadyState::Ready);
    set(FCChangedState::Unchanged);
    emit progress(100, "Complete");

    return result;
}

// ============================================================================
// ТРАНСФОРМАЦИИ
// ============================================================================

void FCSVGImageParser::pushTransform(const QXmlStreamReader &reader)
{
    QString tStr = reader.attributes().value("transform").toString();
    QTransform cur = _transformStack.isEmpty() ? QTransform() : _transformStack.top();
    if (!tStr.isEmpty()) {
        cur = cur * parseTransformString(tStr);
    }
    _transformStack.push(cur);
}

void FCSVGImageParser::popTransform()
{
    if (_transformStack.size() > 1) {
        _transformStack.pop();
    }
}

void FCSVGImageParser::applyCurrentTransform(QVector<FC3DPoint> &points) const
{
    if (_transformStack.isEmpty()) {
        return;
    }
    const QTransform &m = _transformStack.top();
    if (m.isIdentity()) {
        return;
    }
    for (auto &p : points) {
        qreal nx = m.m11() * p.x() + m.m21() * p.y() + m.dx();
        qreal ny = m.m12() * p.x() + m.m22() * p.y() + m.dy();
        p.setX(nx);
        p.setY(ny);
    }
}

QTransform FCSVGImageParser::parseTransformString(const QString &str) const
{
    QTransform res;
    static const QRegularExpression funcRx(R"(([a-zA-Z]+)\(([^)]*)\))");
    auto it = funcRx.globalMatch(str);

    while (it.hasNext()) {
        auto m = it.next();
        QString f = m.captured(1).toLower();
        QStringList args = m.captured(2).split(QRegularExpression(R"([\s,]+)"), Qt::SkipEmptyParts);
        QVector<qreal> nums;
        for (const auto &a : args) {
            nums.append(a.toDouble());
        }

        QTransform t;
        if (f == "matrix" && nums.size() == 6) {
            t = QTransform(nums[0], nums[1], nums[2], nums[3], nums[4], nums[5]);
        } else if (f == "translate") {
            t.translate(nums.value(0), nums.value(1));
        } else if (f == "scale") {
            t.scale(nums.value(0), nums.value(1, nums[0]));
        } else if (f == "rotate") {
            qreal a = nums.value(0);
            if (nums.size() == 3) {
                t.translate(nums[1], nums[2]);
                t.rotate(a);
                t.translate(-nums[1], -nums[2]);
            } else {
                t.rotate(a);
            }
        } else if (f == "skewX") {
            t.shear(qTan(qDegreesToRadians(nums.value(0))), 0);
        } else if (f == "skewY") {
            t.shear(0, qTan(qDegreesToRadians(nums.value(0))));
        }
        res = res * t;
    }
    return res;
}

// ============================================================================
// ПАРСИНГ ПУТЕЙ И ДУГ
// ============================================================================

void FCSVGImageParser::parsePathData(const QString &pathData, QVector<FC3DPoint> &points)
{
    int pos = 0;
    qreal curX = 0;
    qreal curY = 0;
    qreal startX = 0;
    qreal startY = 0;

    static const QRegularExpression cmdRx(R"([MmZzLlHhVvCcSsQqTtAa])");
    static const QRegularExpression numRx(R"(-?\d*\.?\d+(?:[eE][+-]?\d+)?)");
    const int len = pathData.length();

    while (pos < len) {
        QRegularExpressionMatch cmdM = cmdRx.match(pathData, pos);
        if (!cmdM.hasMatch()) {
            break;
        }

        QString cmd = cmdM.captured(0);
        pos = cmdM.capturedEnd(0);

        QVector<qreal> params;
        while (pos < len) {
            QRegularExpressionMatch numM = numRx.match(pathData, pos);
            if (!numM.hasMatch()) {
                break;
            }
            params.append(numM.captured(0).toDouble());
            pos = numM.capturedEnd(0);
        }

        int idx = 0;
        bool rel = cmd.isLower();

        switch (cmd.toUpper().at(0).toLatin1()) {
        case 'M':
            while (idx + 1 < params.size()) {
                qreal x = params[idx] + (rel ? curX : 0);
                qreal y = params[idx + 1] + (rel ? curY : 0);
                points.append({ x, y, 0 });
                curX = x;
                curY = y;
                if (points.size() == 1) {
                    startX = x;
                    startY = y;
                }
                idx += 2;
            }
            break;
        case 'L':
            while (idx + 1 < params.size()) {
                qreal x = params[idx] + (rel ? curX : 0);
                qreal y = params[idx + 1] + (rel ? curY : 0);
                points.append({ x, y, 0 });
                curX = x;
                curY = y;
                idx += 2;
            }
            break;
        case 'H':
            while (idx < params.size()) {
                qreal x = params[idx] + (rel ? curX : 0);
                points.append({ x, curY, 0 });
                curX = x;
                idx++;
            }
            break;
        case 'V':
            while (idx < params.size()) {
                qreal y = params[idx] + (rel ? curY : 0);
                points.append({ curX, y, 0 });
                curY = y;
                idx++;
            }
            break;
        case 'C':
            while (idx + 5 < params.size()) {
                qreal c1x = params[idx] + (rel ? curX : 0);
                qreal c1y = params[idx + 1] + (rel ? curY : 0);
                qreal c2x = params[idx + 2] + (rel ? curX : 0);
                qreal c2y = params[idx + 3] + (rel ? curY : 0);
                qreal ex = params[idx + 4] + (rel ? curX : 0);
                qreal ey = params[idx + 5] + (rel ? curY : 0);

                for (int i = 1; i <= 10; ++i) {
                    qreal t = i / 10.0;
                    qreal mt = 1 - t;
                    qreal bx = mt * mt * mt * curX + 3 * mt * mt * t * c1x +
                               3 * mt * t * t * c2x + t * t * t * ex;
                    qreal by = mt * mt * mt * curY + 3 * mt * mt * t * c1y +
                               3 * mt * t * t * c2y + t * t * t * ey;
                    points.append({ bx, by, 0 });
                }
                curX = ex;
                curY = ey;
                idx += 6;
            }
            break;
        case 'A':
            while (idx + 6 < params.size()) {
                qreal rx = params[idx];
                qreal ry = params[idx + 1];
                qreal rot = params[idx + 2];
                bool la = params[idx + 3] != 0;
                bool sw = params[idx + 4] != 0;
                qreal ex = params[idx + 5] + (rel ? curX : 0);
                qreal ey = params[idx + 6] + (rel ? curY : 0);

                if (!points.isEmpty()) {
                    appendArc(points, rx, ry, rot, la, sw, ex, ey);
                } else {
                    points.append({ ex, ey, 0 });
                }
                curX = ex;
                curY = ey;
                idx += 7;
            }
            break;
        case 'Z':
            if (!points.isEmpty()) {
                points.append({ startX, startY, 0 });
                curX = startX;
                curY = startY;
            }
            break;
        default:
            break;
        }
    }
}

void FCSVGImageParser::appendArc(QVector<FC3DPoint> &points, qreal rx, qreal ry,
                                 qreal xAxisRotation, bool largeArc, bool sweep,
                                 qreal endX, qreal endY) const
{
    if (rx == 0 || ry == 0) {
        points.append({ endX, endY, 0 });
        return;
    }

    qreal phi = qDegreesToRadians(xAxisRotation);
    qreal cosP = qCos(phi);
    qreal sinP = qSin(phi);

    qreal x1 = points.last().x();
    qreal y1 = points.last().y();
    rx = qAbs(rx);
    ry = qAbs(ry);

    qreal dx2 = (x1 - endX) / 2.0;
    qreal dy2 = (y1 - endY) / 2.0;
    qreal x1_ = cosP * dx2 + sinP * dy2;
    qreal y1_ = -sinP * dx2 + cosP * dy2;

    qreal lambda = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
    if (lambda > 1.0) {
        qreal s = qSqrt(lambda);
        rx *= s;
        ry *= s;
    }

    qreal rSq = rx * rx * ry * ry;
    qreal num = rSq - (rx * rx * y1_ * y1_) - (ry * ry * x1_ * x1_);
    qreal den = rx * rx * y1_ * y1_ + ry * ry * x1_ * x1_;
    qreal coef = qSqrt(qMax(qreal(0.0), num / den));
    if (largeArc == sweep) {
        coef = -coef;
    }

    qreal cx_ = coef * (rx * y1_) / ry;
    qreal cy_ = coef * (-ry * x1_) / rx;
    qreal cx = cosP * cx_ - sinP * cy_ + (x1 + endX) / 2.0;
    qreal cy = sinP * cx_ + cosP * cy_ + (y1 + endY) / 2.0;

    qreal startA = qAtan2((y1_ - cy_) / ry, (x1_ - cx_) / rx);
    qreal endA = qAtan2((-y1_ - cy_) / ry, (-x1_ - cx_) / rx);
    qreal sweepA = endA - startA;

    if (!sweep && sweepA > 0) {
        sweepA -= 2.0 * M_PI;
    }
    if (sweep && sweepA < 0) {
        sweepA += 2.0 * M_PI;
    }

    int segs = qMax(1, qCeil(qAbs(sweepA) / (M_PI / 18.0)));
    for (int i = 1; i <= segs; ++i) {
        qreal t = startA + (sweepA * i) / segs;
        qreal ex = cosP * rx * qCos(t) - sinP * ry * qSin(t) + cx;
        qreal ey = sinP * rx * qCos(t) + cosP * ry * qSin(t) + cy;
        points.append({ ex, ey, 0 });
    }
}

// ============================================================================
// ПАРСИНГ ЭЛЕМЕНТОВ
// ============================================================================

bool FCSVGImageParser::parsePathElement(QXmlStreamReader &reader,
                                        FCImageBinaryContainer::BinaryFigure &figure)
{
    QString d = reader.attributes().value("d").toString();
    if (d.isEmpty()) {
        return false;
    }

    figure.pathData = d;
    parsePathData(d, figure.points);
    applyCurrentTransform(figure.points);

    QString c = reader.attributes().value("stroke").toString();
    if (!c.isEmpty()) {
        figure.color = QColor(c);
    } else {
        c = reader.attributes().value("fill").toString();
        if (!c.isEmpty() && c != "none") {
            figure.color = QColor(c);
            figure.isFilled = true;
        }
    }

    QString w = reader.attributes().value("stroke-width").toString();
    if (!w.isEmpty()) {
        figure.thickness = w.toFloat();
    }

    QString id = reader.attributes().value("id").toString();
    if (!id.isEmpty()) {
        figure.pathData.prepend("#" + id + " ");
    }

    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseSvgElement(QXmlStreamReader &reader,
                                       FCImageBinaryContainer::BinaryLayer &layer)
{
    Q_UNUSED(layer)

    QString viewBoxStr = reader.attributes().value("viewBox").toString();
    if (!viewBoxStr.isEmpty()) {
        QStringList parts = viewBoxStr.split(QRegularExpression(R"([\s,]+)"), Qt::SkipEmptyParts);
        if (parts.size() >= 4) {
            FC2DSize viewBox;
            viewBox.setWidth(parts[2].toFloat());
            viewBox.setHeight(parts[3].toFloat());
            _container->setViewBox(viewBox);
        }
    }

    QString w = reader.attributes().value("width").toString();
    QString h = reader.attributes().value("height").toString();
    if (!w.isEmpty() && !h.isEmpty()) {
        FCImageBinaryContainer::Metadata meta = _container->metadata();
        meta.imageSize = { w.toFloat(), h.toFloat() };
        _container->setMetadata(meta);
    }

    return true;
}

void FCSVGImageParser::parseGroupElement(QXmlStreamReader &reader,
                                         FCImageBinaryContainer::BinaryLayer &parentLayer,
                                         QStack<FCImageBinaryContainer::BinaryLayer *> &layerStack)
{
    QString id = reader.attributes().value("id").toString();
    QString label = reader.attributes().value("inkscape:label").toString();
    QString mode = reader.attributes().value("inkscape:groupmode").toString();

    if (mode == "layer") {
        FCImageBinaryContainer::BinaryLayer newLayer;
        newLayer.name = label.isEmpty() ? id : label;
        newLayer.inkscapeId = id;
        newLayer.isVisible = true;

        QString style = reader.attributes().value("style").toString();
        if (style.contains("display:none", Qt::CaseInsensitive)) {
            newLayer.isVisible = false;
        }

        _container->addLayer(newLayer);
        layerStack.push(&_container->layerRef(_container->layerCount() - 1));
    }

    Q_UNUSED(reader)
    Q_UNUSED(parentLayer)
}

bool FCSVGImageParser::parseRectElement(QXmlStreamReader &reader,
                                        FCImageBinaryContainer::BinaryFigure &figure)
{
    qreal x = reader.attributes().value("x").toDouble();
    qreal y = reader.attributes().value("y").toDouble();
    qreal w = reader.attributes().value("width").toDouble();
    qreal h = reader.attributes().value("height").toDouble();
    qreal rx = reader.attributes().value("rx").toDouble();
    qreal ry = reader.attributes().value("ry").toDouble();

    if (w <= 0 || h <= 0) {
        return false;
    }

    if (rx > 0 || ry > 0) {
        parseRoundedRect(x, y, w, h, rx, ry, figure.points);
    } else {
        figure.points.append({ x, y, 0 });
        figure.points.append({ x + w, y, 0 });
        figure.points.append({ x + w, y + h, 0 });
        figure.points.append({ x, y + h, 0 });
        figure.points.append({ x, y, 0 });
    }

    applyCurrentTransform(figure.points);
    figure.isFilled = true;
    applyStyleAttributes(reader, figure);
    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseCircleElement(QXmlStreamReader &reader,
                                          FCImageBinaryContainer::BinaryFigure &figure)
{
    qreal cx = reader.attributes().value("cx").toDouble();
    qreal cy = reader.attributes().value("cy").toDouble();
    qreal r = reader.attributes().value("r").toDouble();
    if (r <= 0) {
        return false;
    }

    for (int i = 0; i <= 36; ++i) {
        qreal a = (2.0 * M_PI * i) / 36.0;
        figure.points.append({ cx + r * qCos(a), cy + r * qSin(a), 0 });
    }

    applyCurrentTransform(figure.points);
    figure.isFilled = true;
    applyStyleAttributes(reader, figure);
    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseEllipseElement(QXmlStreamReader &reader,
                                           FCImageBinaryContainer::BinaryFigure &figure)
{
    qreal cx = reader.attributes().value("cx").toDouble();
    qreal cy = reader.attributes().value("cy").toDouble();
    qreal rx = reader.attributes().value("rx").toDouble();
    qreal ry = reader.attributes().value("ry").toDouble();
    if (rx <= 0 || ry <= 0) {
        return false;
    }

    for (int i = 0; i <= 36; ++i) {
        qreal a = (2.0 * M_PI * i) / 36.0;
        figure.points.append({ cx + rx * qCos(a), cy + ry * qSin(a), 0 });
    }

    applyCurrentTransform(figure.points);
    figure.isFilled = true;
    applyStyleAttributes(reader, figure);
    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parsePolygonElement(QXmlStreamReader &reader,
                                           FCImageBinaryContainer::BinaryFigure &figure)
{
    QString pts = reader.attributes().value("points").toString();
    if (pts.isEmpty()) {
        return false;
    }

    parsePointsAttribute(pts, figure.points);
    if (!figure.points.isEmpty()) {
        figure.points.append(figure.points.first());
    }

    applyCurrentTransform(figure.points);
    figure.isFilled = true;
    applyStyleAttributes(reader, figure);
    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parsePolylineElement(QXmlStreamReader &reader,
                                            FCImageBinaryContainer::BinaryFigure &figure)
{
    QString pts = reader.attributes().value("points").toString();
    if (pts.isEmpty()) {
        return false;
    }

    parsePointsAttribute(pts, figure.points);
    applyCurrentTransform(figure.points);
    figure.isFilled = false;
    applyStyleAttributes(reader, figure);
    return !figure.points.isEmpty();
}

bool FCSVGImageParser::parseLineElement(QXmlStreamReader &reader,
                                        FCImageBinaryContainer::BinaryFigure &figure)
{
    figure.points.append({ reader.attributes().value("x1").toDouble(),
                           reader.attributes().value("y1").toDouble(), 0 });
    figure.points.append({ reader.attributes().value("x2").toDouble(),
                           reader.attributes().value("y2").toDouble(), 0 });

    applyCurrentTransform(figure.points);
    figure.isFilled = false;
    applyStyleAttributes(reader, figure);
    return !figure.points.isEmpty();
}

void FCSVGImageParser::updateProgress(int percent, const QString &stage)
{
    emit progress(percent, stage);
}

void FCSVGImageParser::parsePointsAttribute(const QString &pts, QVector<FC3DPoint> &points)
{
    static const QRegularExpression numRx(R"(-?\d*\.?\d+(?:[eE][+-]?\d+)?)");
    auto it = numRx.globalMatch(pts);
    QVector<qreal> coords;

    while (it.hasNext()) {
        coords.append(it.next().captured(0).toDouble());
    }

    for (int i = 0; i + 1 < coords.size(); i += 2) {
        points.append({ coords[i], coords[i + 1], 0 });
    }
}

void FCSVGImageParser::parseRoundedRect(qreal x, qreal y, qreal w, qreal h,
                                        qreal rx, qreal ry, QVector<FC3DPoint> &points)
{
    if (rx <= 0) {
        rx = ry;
    }
    if (ry <= 0) {
        ry = rx;
    }
    rx = qMin(rx, w / 2.0);
    ry = qMin(ry, h / 2.0);

    const int seg = 8;
    auto appendC = [&](qreal cx, qreal cy, qreal sA, qreal eA) {
        for (int i = 0; i <= seg; ++i) {
            qreal a = sA + (eA - sA) * (i / static_cast<qreal>(seg));
            points.append({ cx + rx * qCos(a), cy + ry * qSin(a), 0 });
        }
    };

    appendC(x + rx, y + ry, M_PI, 1.5 * M_PI);
    appendC(x + w - rx, y + ry, 1.5 * M_PI, 2.0 * M_PI);
    appendC(x + w - rx, y + h - ry, 0, 0.5 * M_PI);
    appendC(x + rx, y + h - ry, 0.5 * M_PI, M_PI);
}

void FCSVGImageParser::applyStyleAttributes(QXmlStreamReader &reader,
                                            FCImageBinaryContainer::BinaryFigure &fig)
{
    QString s = reader.attributes().value("stroke").toString();
    if (!s.isEmpty()) {
        fig.color = QColor(s);
    }

    QString f = reader.attributes().value("fill").toString();
    if (!f.isEmpty() && f != "none") {
        fig.isFilled = true;
        if (s.isEmpty()) {
            fig.color = QColor(f);
        }
    }

    QString w = reader.attributes().value("stroke-width").toString();
    if (!w.isEmpty()) {
        fig.thickness = w.toFloat();
    }
}

// ============================================================================
// ЭКСПОРТ В G-CODE ДЛЯ ОТЛАДКИ
// ============================================================================

bool FCSVGImageParser::exportToGCode(const QString &filePath, qreal feedRate, qreal safeHeight) const
{
    if (!_container || !is(FCReadyState::Ready)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(4);

    out << "%\n";
    out << "G90 G21\n";
    out << "G17\n";
    out << "G0 Z" << safeHeight << "\n";

    for (const auto &layer : _container->layers()) {
        out << "; === Layer: " << layer.name << " ===\n";
        for (const auto &fig : layer.figures) {
            if (fig.points.isEmpty()) {
                continue;
            }
            out << "; Points: " << fig.points.size()
                << (fig.isFilled ? " (filled)" : "") << "\n";

            const auto &s = fig.points.first();
            out << QString("G0 X%1 Y%2\n").arg(s.x()).arg(s.y());
            out << QString("G1 Z0 F%1\n").arg(feedRate / 2.0);

            for (int i = 1; i < fig.points.size(); ++i) {
                const auto &p = fig.points[i];
                out << QString("G1 X%1 Y%2 F%3\n").arg(p.x()).arg(p.y()).arg(feedRate);
            }
            out << QString("G0 Z%1\n").arg(safeHeight);
        }
    }

    out << "G0 X0 Y0\n";
    out << "M2\n";
    out << "%\n";

    return true;
}
