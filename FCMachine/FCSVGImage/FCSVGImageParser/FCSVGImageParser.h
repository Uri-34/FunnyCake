#ifndef FC_SVG_IMAGE_PARSER_H
#define FC_SVG_IMAGE_PARSER_H

#include <QObject>
#include <QThread>
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

#include "FCState.h"
#include "FC2DPoint.h"
#include "FC2DSize.h"
#include "FC3DPoint.h"
#include "FCSVGImageContainer.h"

/// @brief Минимальный набор для парсера
using FCParserState = FCStateT<FCReadyState, FCPlayState, FCChangedState, FCErrorType>;

/**
* @class FCSVGImageParser
* @brief Парсер SVG-файлов в бинарный формат для генерации G-кода
*/
class FCSVGImageParser
    : public QObject
{
Q_OBJECT
public:
    /**
    * @struct ParserSettings
    * @brief Настройки парсера для управления качеством и оптимизацией
    */
    struct ParserSettings
    {
        qreal tolerance;              ///< Допуск упрощения путей (мм)
        bool simplifyPaths;           ///< Применять алгоритм упрощения кривых
        bool mergeAdjacent;           ///< Объединять смежные коллинеарные сегменты
        bool removeDuplicates;        ///< Удалять дублирующиеся точки в контурах
        qreal minSegmentLength;       ///< Минимальная длина сегмента (мм)
        bool extractText;             ///< Преобразовывать текстовые элементы в контуры
        bool flattenGroups;           ///< Игнорировать группировку (<g>)
        QColor backgroundColor;       ///< Цвет фона для обработки прозрачных областей
        bool ignoreHiddenLayers;      ///< Пропускать слои со style="display:none"

        ParserSettings()
        : tolerance(0.01)
        , simplifyPaths(false)
        , mergeAdjacent(false)
        , removeDuplicates(true)
        , minSegmentLength(0.0)
        , extractText(false)
        , flattenGroups(false)
        , backgroundColor(Qt::white)
        , ignoreHiddenLayers(true)
        {}
    };

    /**
    * @struct ParseResult
    * @brief Результат парсинга SVG-файла
    */
    struct ParseResult
    {
        bool success;                 ///< Флаг успешного завершения парсинга
        QString errorMessage;         ///< Описание ошибки (пусто если success=true)
        QString sourceFile;           ///< Путь к исходному SVG-файлу
        quint32 figuresParsed;        ///< Количество распарсенных фигур
        quint32 layersParsed;         ///< Количество распарсенных слоёв
        quint64 pointsTotal;          ///< Общее количество точек во всех контурах
        qint64 parseTimeMs;           ///< Время выполнения парсинга (мс)

        ParseResult()
        : success(false)
        , figuresParsed(0)
        , layersParsed(0)
        , pointsTotal(0)
        , parseTimeMs(0)
        {}

        [[nodiscard]] bool isValid() const noexcept { return success && errorMessage.isEmpty(); }
    };

    // Конструкторы / Деструктор
    explicit FCSVGImageParser(QObject *parent = nullptr);
    explicit FCSVGImageParser(const ParserSettings &settings, QObject *parent = nullptr);
    ~FCSVGImageParser() override;

    // ПРОВЕРКА СОСТОЯНИЙ
    [[nodiscard]] inline bool is(FCReadyState state) const noexcept { return _state.is(state); }
    [[nodiscard]] inline bool is(FCPlayState state) const noexcept { return _state.is(state); }
    [[nodiscard]] inline bool is(FCChangedState state) const noexcept { return _state.is(state); }
    [[nodiscard]] inline bool is(FCErrorType type) const noexcept { return _state.is(type); }

    // УСТАНОВКА СОСТОЯНИЙ
    inline void set(FCReadyState state);
    inline void set(FCPlayState state);
    inline void set(FCChangedState state);
    inline void set(FCErrorType type);

    // Управление настройками
    void setSettings(const ParserSettings &settings);
    [[nodiscard]] ParserSettings settings() const noexcept;

    // Методы парсинга
    [[nodiscard]] ParseResult parse(const QString &filePath);
    [[nodiscard]] ParseResult parse(const QByteArray &data, const QString &name = QStringLiteral("memory"));
    void parseAsync(const QString &filePath);
    void cancel();
    [[nodiscard]] bool isParsing() const noexcept;
    [[nodiscard]] FCSVGImageContainer* container() const noexcept;

protected:
    // Внутренние методы парсинга
    [[nodiscard]] ParseResult parseSvgInternal(const QByteArray &data);
    [[nodiscard]] bool parsePathElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);
    bool parseSvgElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryLayer &layer);
    [[nodiscard]] bool parseRectElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);
    [[nodiscard]] bool parseCircleElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);
    [[nodiscard]] bool parseEllipseElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);
    [[nodiscard]] bool parsePolygonElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);
    [[nodiscard]] bool parsePolylineElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);
    [[nodiscard]] bool parseLineElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);
    void parseGroupElement(QXmlStreamReader &reader, FCSVGImageContainer::BinaryLayer &parentLayer, QStack<FCSVGImageContainer::BinaryLayer*> &layerStack);
    void updateProgress(int percent, const QString &stage);

    // Вспомогательные функции оптимизации геометрии
private:
    void removeDuplicatePoints(QVector<FC2DPoint> &points) noexcept;
    void removeDuplicatePoints(QVector<FC3DPoint> &points) noexcept;
    void simplifyPath(QVector<FC2DPoint> &points, qreal tolerance) noexcept;
    void simplifyPath(QVector<FC3DPoint> &points, qreal tolerance) noexcept;
    void mergeAdjacentSegments(QVector<FC2DPoint> &points) noexcept;
    void mergeAdjacentSegments(QVector<FC3DPoint> &points) noexcept;
    [[nodiscard]] static qreal distance2D(const FC3DPoint &a, const FC3DPoint &b) noexcept;
    void parsePathData(const QString &pathData, QVector<FC3DPoint> &points);
    void parsePointsAttribute(const QString &pointsStr, QVector<FC3DPoint> &points);
    void parseRoundedRect(qreal x, qreal y, qreal width, qreal height, qreal rx, qreal ry, QVector<FC3DPoint> &points);
    void applyStyleAttributes(QXmlStreamReader &reader, FCSVGImageContainer::BinaryFigure &figure);

    // ИНИЦИАЛИЗАЦИЯ МАШИНЫ СОСТОЯНИЙ
    void initStateMachine();

    // ЧЛЕНЫ ДАННЫХ
    ParserSettings _settings;
    FCSVGImageContainer *_container = nullptr;
    QFuture<void> _future;
    QAtomicInt _isParsing{0};
    QString _currentFile;

    // Машина состояний
    QStateMachine *_stateMachine = nullptr;

    // Локальное состояние
    FCParserState _state;

signals:
    void started(const QString &filePath);
    void finished(const FCSVGImageParser::ParseResult &result);
    void progress(int percent, const QString &stage);
    void error(const QString &name, const QString &message);
    void containerReady(FCSVGImageContainer *container);

    // Сигналы изменений состояний
    void readyStateChanged(FCReadyState state);
    void playStateChanged(FCPlayState state);
    void changedStateChanged(FCChangedState state);
    void errorTypeChanged(FCErrorType type);
};

#endif // FC_SVG_IMAGE_PARSER_H
