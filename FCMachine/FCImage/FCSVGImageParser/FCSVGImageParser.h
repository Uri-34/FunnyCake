#ifndef FC_SVG_IMAGE_PARSER_H
#define FC_SVG_IMAGE_PARSER_H

#include <QObject>
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
#include <QTransform>

#include "FCState.h"
#include "FC2DPoint.h"
#include "FC2DSize.h"
#include "FC3DPoint.h"
#include "FCImageBinaryContainer.h"

using FCParserState = FCStateT<FCReadyState, FCPlayState, FCChangedState, FCErrorType>;

class FCSVGImageParser : public QObject
{
    Q_OBJECT

public:
    struct ParserSettings
    {
        qreal tolerance = 0.01;
        bool simplifyPaths = false;
        bool mergeAdjacent = false;
        bool removeDuplicates = true;
        qreal minSegmentLength = 0.0;
        bool extractText = false;
        bool flattenGroups = false;
        QColor backgroundColor = Qt::white;
        bool ignoreHiddenLayers = true;
    };

    struct ParseResult
    {
        bool success = false;
        QString errorMessage;
        QString sourceFile;
        quint32 figuresParsed = 0;
        quint32 layersParsed = 0;
        quint64 pointsTotal = 0;
        qint64 parseTimeMs = 0;
        [[nodiscard]] bool isValid() const noexcept { return success && errorMessage.isEmpty(); }
    };

    explicit FCSVGImageParser(QObject *parent = nullptr);
    explicit FCSVGImageParser(const ParserSettings &settings, QObject *parent = nullptr);
    ~FCSVGImageParser() override;

    [[nodiscard]] inline bool is(FCReadyState state) const noexcept { return _state.is(state); }
    [[nodiscard]] inline bool is(FCPlayState state) const noexcept { return _state.is(state); }
    [[nodiscard]] inline bool is(FCChangedState state) const noexcept { return _state.is(state); }
    [[nodiscard]] inline bool is(FCErrorType type) const noexcept { return _state.is(type); }

    inline void set(FCReadyState state);
    inline void set(FCPlayState state);
    inline void set(FCChangedState state);
    inline void set(FCErrorType type);

    void setSettings(const ParserSettings &settings);
    [[nodiscard]] ParserSettings settings() const noexcept;

    [[nodiscard]] ParseResult parse(const QString &filePath);
    [[nodiscard]] ParseResult parse(const QByteArray &data, const QString &name = QStringLiteral("memory"));
    void parseAsync(const QString &filePath);
    [[nodiscard]] bool isParsing() const noexcept;
    [[nodiscard]] FCImageBinaryContainer* container() const noexcept;
    [[nodiscard]] bool exportToGCode(const QString &filePath, qreal feedRate = 1000.0, qreal safeHeight = 5.0) const;

public slots:
    void cancel();

protected:
    [[nodiscard]] ParseResult parseSvgInternal(const QByteArray &data);
    [[nodiscard]] bool parsePathElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);
    bool parseSvgElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryLayer &layer);
    [[nodiscard]] bool parseRectElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);
    [[nodiscard]] bool parseCircleElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);
    [[nodiscard]] bool parseEllipseElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);
    [[nodiscard]] bool parsePolygonElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);
    [[nodiscard]] bool parsePolylineElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);
    [[nodiscard]] bool parseLineElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);
    void parseGroupElement(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryLayer &parentLayer, QStack<FCImageBinaryContainer::BinaryLayer*> &layerStack);
    void updateProgress(int percent, const QString &stage);

private:
    void removeDuplicatePoints(QVector<FC2DPoint> &points) noexcept;
    void removeDuplicatePoints(QVector<FC3DPoint> &points) noexcept;
    void simplifyPath(QVector<FC3DPoint> &points, qreal tolerance) noexcept;
    void mergeAdjacentSegments(QVector<FC3DPoint> &points) noexcept;
    [[nodiscard]] static qreal distance2D(const FC3DPoint &a, const FC3DPoint &b) noexcept;

    void parsePathData(const QString &pathData, QVector<FC3DPoint> &points);
    void parsePointsAttribute(const QString &pointsStr, QVector<FC3DPoint> &points);
    void parseRoundedRect(qreal x, qreal y, qreal width, qreal height, qreal rx, qreal ry, QVector<FC3DPoint> &points);
    void applyStyleAttributes(QXmlStreamReader &reader, FCImageBinaryContainer::BinaryFigure &figure);

    void pushTransform(const QXmlStreamReader &reader);
    void popTransform();
    void applyCurrentTransform(QVector<FC3DPoint> &points) const;
    [[nodiscard]] QTransform parseTransformString(const QString &str) const;
    void appendArc(QVector<FC3DPoint> &points, qreal rx, qreal ry, qreal xAxisRotation,
                   bool largeArc, bool sweep, qreal endX, qreal endY) const;

    ParserSettings _settings;
    FCImageBinaryContainer *_container = nullptr;
    QFuture<void> _future;
    QAtomicInt _isParsing{0};
    QString _currentFile;
    FCParserState _state;
    QStack<QTransform> _transformStack;

signals:
    void started(const QString &filePath);
    void finished(const FCSVGImageParser::ParseResult &result);
    void progress(int percent, const QString &stage);
    void error(const QString &name, const QString &message);
    void containerReady(FCImageBinaryContainer *container);
    void readyStateChanged(FCReadyState state);
    void playStateChanged(FCPlayState state);
    void changedStateChanged(FCChangedState state);
    void errorTypeChanged(FCErrorType type);
};

#endif // FC_SVG_IMAGE_PARSER_H
