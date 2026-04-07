#ifndef FC_SVG_IMAGE_CONTAINER_H
#define FC_SVG_IMAGE_CONTAINER_H

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
#include "FCState.h"

/**
 * @class FCSVGImageContainer
 * @brief Потокобезопасный контейнер для хранения результатов парсинга SVG в бинарный формат
 *
 * @note ВСЁ состояние управляется через FCStateT<> (не FCConditionObject!)
 * @note Сигналы эмитятся в классах-владельцах (FCDisplay, FCMachine)
 * @threadsafe
 */
class FCSVGImageContainer : public QObject
{
    Q_OBJECT

public:
    const FCSVGImageContainerState FCSVGImageContainerDefaultState {FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Unchanged, FCErrorState::None, FCErrorType::None};

    // СТРУКТУРЫ ДАННЫХ
    struct Metadata
    {
        QString sourceFile;
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
        QVector<quint8> pointTypes;
        bool isFilled = false;
        QVector<FC2DPoint> fillPath;
        FC2DPoint boundingBoxMin;
        FC2DPoint boundingBoxMax;
        qreal length = 0.0;
        qreal area = 0.0;
        QString pathData;
        QTransform transform;

        [[nodiscard]] inline bool isValid() const noexcept { return points.size() >= 2; }
        void recalculateMetrics();
    };

    struct BinaryLayer
    {
        quint32 index = 0;
        QString name;
        QString inkscapeId;
        qreal zPosition = 0.0;
        qreal thickness = 0.0;
        QVector<BinaryFigure> figures;
        QColor layerColor;
        bool isVisible = true;
        bool isPrinted = false;
        QTransform transform;

        [[nodiscard]] inline bool hasFigures() const noexcept { return !figures.isEmpty(); }
        quint32 appendFigure(const BinaryFigure &figure);
    };

    // ========================================================================
    // КОНСТРУКТОРЫ / ДЕСТРУКТОР
    // ========================================================================

    explicit FCSVGImageContainer(QObject *parent = nullptr);
    explicit FCSVGImageContainer(const QString &name, QObject *parent = nullptr);
    FCSVGImageContainer(const FCSVGImageContainer &other, QObject *parent = nullptr);
    ~FCSVGImageContainer() override;

    // ========================================================================
    // УПРАВЛЕНИЕ ЗАПИСЬЮ
    // ========================================================================

    [[nodiscard]] bool beginWrite();
    void endWrite();
    [[nodiscard]] QWriteLocker writeLocker();

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

    // ========================================================================
    // ЧТЕНИЕ ДАННЫХ (const-методы, потокобезопасные)
    // ========================================================================

    [[nodiscard]] quint32 layerCount() const noexcept;
    [[nodiscard]] BinaryLayer layer(quint32 index) const;
    [[nodiscard]] QVector<BinaryLayer> layers() const;
    [[nodiscard]] const QVector<BinaryLayer> &layersRef() const;
    [[nodiscard]] BinaryFigure figure(quint32 layerIndex, quint32 figureIndex) const;
    [[nodiscard]] QVector<BinaryFigure> figures(quint32 layerIndex) const;
    [[nodiscard]] Metadata metadata() const noexcept;
    [[nodiscard]] FC2DSize viewBox() const noexcept;
    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] quint32 totalFigures() const noexcept;
    [[nodiscard]] quint64 memorySize() const noexcept;

    // ========================================================================
    // УПРАВЛЕНИЕ СОСТОЯНИЕМ (FCStateT<>)
    // ========================================================================

    /// @brief Получить текущее состояние контейнера
    [[nodiscard]] inline FCSVGImageContainerState state() const noexcept { return _state; }

    /// @brief Установить состояние контейнера
    inline void setState(const FCSVGImageContainerState &state) { _state = state; }

    /// @brief Установить одно или несколько состояний (вариативный шаблон)
    template<typename First, typename... Rest>
    inline void setState(First first, Rest... rest)
    {
        _state.set(first, rest...);
    }

    /// @brief Проверить состояние
    template<typename State>
    [[nodiscard]] inline bool is(State value) const noexcept
    {
        return _state.is(value);
    }

    // УТИЛИТЫ
    [[nodiscard]] QReadLocker readLocker() const;
    [[nodiscard]] bool canRead() const noexcept;
    QSharedPointer<FCSVGImageContainer> snapshot() const;
    [[nodiscard]] qint32 findLayerByName(const QString &name) const;
    [[nodiscard]] QVector<QPair<quint32, quint32>> findFiguresByColor(const QColor &color, quint8 tolerance = 0) const;
    [[nodiscard]] inline FC2DSize imageSize() const noexcept { return _metadata.imageSize; }
    FCSVGImageContainer& operator=(const FCSVGImageContainer &other);

signals:
//    void containerReady();
//    void containerError(FCErrorType error, const QString &details);
    void writeProgress(int percent);
    void layerAdded(quint32 layerIndex);
    void figureAdded(quint32 layerIndex, quint32 figureIndex);

    /// @brief Сигнал изменения состояния (использует FCStateT<>)
    void condition(const FCSVGImageContainerState &state, const QString &details = QString());

private:
    QVector<BinaryLayer> _layers;
    Metadata _metadata;
    mutable QReadWriteLock _lock;
//    QAtomicInt _readyState;
    QAtomicInt _writeInProgress;
    quint32 _figureCounter = 1;
    FC2DSize _viewBox;

    /// @brief Состояние контейнера (FCStateT<>)
    FCSVGImageContainerState _state;

    void validateLayerIndex(quint32 index) const;
    void calculateStatistics();
    [[nodiscard]] quint32 generateFigureId();
    quint32 addLayerInternal(const BinaryLayer &layer);
};

// INLINE РЕАЛИЗАЦИИ
inline quint32 FCSVGImageContainer::layerCount() const noexcept { return static_cast<quint32>(_layers.size()); }
inline bool FCSVGImageContainer::isReady() const noexcept { return _state.is(FCReadyState::Ready); }
inline bool FCSVGImageContainer::canRead() const noexcept { return _writeInProgress.loadRelaxed() == 0; }
inline quint32 FCSVGImageContainer::totalFigures() const noexcept { return _metadata.figureCount; }
inline FC2DSize FCSVGImageContainer::viewBox() const noexcept { return _viewBox; }
inline const QVector<FCSVGImageContainer::BinaryLayer>& FCSVGImageContainer::layersRef() const { return _layers; }
inline QReadLocker FCSVGImageContainer::readLocker() const { return QReadLocker(&_lock); }
inline QWriteLocker FCSVGImageContainer::writeLocker() { return QWriteLocker(&_lock); }

#endif // FC_SVG_IMAGE_CONTAINER_H
