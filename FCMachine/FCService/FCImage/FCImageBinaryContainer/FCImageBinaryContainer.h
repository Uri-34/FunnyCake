#ifndef FC_IMAGE_BINARY_CONTAINER_H
#define FC_IMAGE_BINARY_CONTAINER_H

#include <QObject>
#include <QVector>
#include <QReadWriteLock>
#include <QAtomicInt>
#include <QFileInfo>
#include <QDateTime>
#include <QColor>
#include <QTransform>
#include <QSharedPointer>

#include "FC2DPoint.h"
#include "FC2DSize.h"
#include "FC3DPoint.h"
#include "FCService.h"

/**
 * @class FCImageBinaryContainer
 * @brief Потокобезопасный контейнер для хранения результатов парсинга векторной графики
 * @note Формат-агностичный: поддерживает SVG, DXF, HPGL и др. через единый интерфейс
 * @threadsafe
 */
class FCImageBinaryContainer
    : public FCService
{
Q_OBJECT
public:
    struct Metadata
    {
        QString sourceFile;
        QString sourceFormat;       ///< Формат исходника (SVG, DXF, HPGL...)
        QFileInfo fileInfo;
        FC2DSize imageSize;
        FC2DSize workingArea;
        FC2DSize viewBox;
        quint32 figureCount = 0;
        quint32 layerCount = 0;
        quint64 totalPoints = 0;
        QColor dominantColor;
        qreal inkCoverage = 0.0;
        QDateTime parsedAt;
        QString parserVersion;
        Metadata() = default;
    };

    struct BinaryFigure
    {
        quint32 id = 0;
        quint32 layerIndex = 0;
        QColor color;
        qreal thickness = 1.0;
        QVector<FC3DPoint> points;
        bool isFilled = false;
        FC2DPoint boundingBoxMin;
        FC2DPoint boundingBoxMax;
        qreal length = 0.0;
        qreal area = 0.0;
        QString pathData;
        QTransform transform;
        [[nodiscard]] inline bool isValid() const noexcept
        {
            return points.size() >= 2;
        }
        void recalculateMetrics();
    };

    struct BinaryLayer
    {
        quint32 index = 0;
        QString name;
        QString inkscapeId;
        QVector<BinaryFigure> figures;
        bool isVisible = true;
        [[nodiscard]] inline bool hasFigures() const noexcept
        {
            return !figures.isEmpty();
        }
        quint32 appendFigure(const BinaryFigure &figure);
    };

    explicit FCImageBinaryContainer(QObject *parent = nullptr);
    explicit FCImageBinaryContainer(const QString &name, QObject *parent = nullptr);
    FCImageBinaryContainer(const FCImageBinaryContainer &other, QObject *parent = nullptr);
    ~FCImageBinaryContainer() override;

    // === Потокобезопасная запись ===
    [[nodiscard]] bool beginWrite();
    void endWrite();
    [[nodiscard]] inline QWriteLocker writeLocker() { return QWriteLocker(&_lock); }

    [[deprecated("Используйте addLayer()")]]
    quint32 appendLayer(const BinaryLayer &layer);

    quint32 addLayer(const BinaryLayer &layer);
    bool appendFigure(quint32 layerIndex, const BinaryFigure &figure);
    BinaryLayer &layerRef(quint32 layerIndex);
    BinaryLayer &defaultLayer();
    void setMetadata(const Metadata &metadata);
    void setViewBox(const FC2DSize &viewBox);
    void clear();
    void setReady();

    // === Потокобезопасное чтение (INLINE-ОПРЕДЕЛЕНИЯ ВНУТРИ КЛАССА) ===
    [[nodiscard]] inline quint32 layerCount() const noexcept { return static_cast<quint32>(_layers.size()); }
    [[nodiscard]] BinaryLayer layer(quint32 index) const;
    [[nodiscard]] QVector<BinaryLayer> layers() const;
    [[nodiscard]] inline const QVector<BinaryLayer> &layersRef() const noexcept { return _layers; }
    [[nodiscard]] BinaryFigure figure(quint32 layerIndex, quint32 figureIndex) const;
    [[nodiscard]] QVector<BinaryFigure> figures(quint32 layerIndex) const;
    [[nodiscard]] Metadata metadata() const noexcept;
    [[nodiscard]] inline FC2DSize viewBox() const noexcept { return _viewBox; }
    [[nodiscard]] inline bool isReady() const noexcept { return _state.is(FCReadyState::Ready); }
    [[nodiscard]] inline bool canRead() const noexcept { return _writeInProgress.loadRelaxed() == 0; }
    [[nodiscard]] inline quint32 totalFigures() const noexcept { return _metadata.figureCount; }
    [[nodiscard]] inline FC2DSize imageSize() const noexcept { return _metadata.imageSize; }
    [[nodiscard]] quint64 memorySize() const noexcept;
    [[nodiscard]] inline QReadLocker readLocker() const { return QReadLocker(&_lock); }

    // === Управление состоянием ===

    inline bool readiness() override { return state().is(FCOpenState::Open, FCReadyState::Ready); }

//    [[nodiscard]] inline FCImageBinaryContainerState state() const noexcept { return _state; }
//    inline void setState(const FCImageBinaryContainerState &state) { _state = state; }

//    template<typename First, typename... Rest>
//    inline void setState(First first, Rest... rest) { _state.set(first, rest...); }

//    template<typename StateEnum>
//    [[nodiscard]] inline bool is(StateEnum value) const noexcept { return _state.is(value); }

//    [[nodiscard]] bool isValid() const noexcept;

    // === Утилиты ===
    QSharedPointer<FCImageBinaryContainer> snapshot() const;
    [[nodiscard]] qint32 findLayerByName(const QString &name) const;
    [[nodiscard]] QVector<QPair<quint32, quint32>> findFiguresByColor(const QColor &color, quint8 tolerance = 0) const;
    FCImageBinaryContainer& operator=(const FCImageBinaryContainer &other);

signals:
    void progress(int percent);
    void layerAdded(quint32 layerIndex);
    void figureAdded(quint32 layerIndex, quint32 figureIndex);

private:
    void validateLayerIndex(quint32 index) const;
    void calculateStatistics();
    quint32 generateFigureId();
    quint32 addLayerInternal(const BinaryLayer &layer);

    QVector<BinaryLayer> _layers;
    Metadata _metadata;
    mutable QReadWriteLock _lock;
    QAtomicInt _writeInProgress{0};
    QAtomicInt _figureCounter{1};
    FC2DSize _viewBox;
};

#endif // FC_IMAGE_BINARY_CONTAINER_H
