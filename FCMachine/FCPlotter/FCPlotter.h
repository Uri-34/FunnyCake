#ifndef FC_PLOTTER_H
#define FC_PLOTTER_H

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QThread>

#include "FCDevice.h"
#include "FCI2CBus.h"
#include "FCSVGImageContainer.h"
#include "FCPlotterHardware.h"

/**
 * @brief Фасад для управления плоттером через сигналы от UI.
 *
 * Предназначен для приёма команд от интерфейса (FCDisplay) и координации
 * работы оборудования. Все публичные слоты безопасны для вызова из любого потока.
 *
 * Типичный сценарий подключения:
 * @code
 * // В FCDisplay (основной поток):
 * auto *plotter = new FCPlotter("/dev/ttyAMA0", i2cBus);
 *
 * // Подключение сигналов ДО запуска потока
 * connect(this, &FCDisplay::play,
 *         plotter, &FCPlotter::start,
 *         Qt::QueuedConnection);  // ← Кросс-поточная доставка
 * connect(this, &FCDisplay::stop,
 *         plotter, &FCPlotter::stop,
 *         Qt::QueuedConnection);
 *
 * // Обратная связь: сигналы плоттера → слоты UI
 * connect(plotter, &FCPlotter::progress,
 *         this, &FCDisplay::onProgress,
 *         Qt::QueuedConnection);
 * connect(plotter, &FCPlotter::result,
 *         this, &FCDisplay::onResult,
 *         Qt::QueuedConnection);
 *
 * // Запуск рабочего потока
 * plotter->startThread();
 * @endcode
 *
 * @threadsafe Все публичные слоты и сигналы безопасны для кросс-поточного использования.
 * @note Класс наследует управление состоянием от FCDevice — используйте
 *       методы is()/set() или прямой доступ к _state.
 * @warning Не вызывайте слоты до успешного startThread() — команды будут
 *          поставлены в очередь, но не обработаны до запуска потока.
 * @see FCDisplay (источник команд), FCPlotterHardware (аппаратный слой)
 */
class FCPlotter
    : public FCDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCPlotter)
public:
    /// @brief Таймаут ожидания завершения рабочего потока (мс)
    static constexpr int THREAD_STOP_TIMEOUT_MS = 5000;

    // --- Конструкторы и деструктор ---
    explicit FCPlotter(QString &portName, FCI2CBus *bus, QObject *parent = nullptr);
    ~FCPlotter() override;

    // --- Доступ к свойствам ---
    [[nodiscard]] inline QString serialNumber() const noexcept { return _serialNumber; }
    [[nodiscard]] bool isThreadRunning() const noexcept;
    [[nodiscard]] QString securityCode(int timeoutMs) override;

    // --- Управление потоком ---
    [[nodiscard]] bool startThread();
    bool stopThread();

    // --- Доступ к состоянию (через базовый класс) ---
    using FCDevice::state;  // Наследуем state(), is(), set() из FCDevice

public slots:
    // ========================================================================
    // СЛОТЫ ДЛЯ ПРИЁМА КОМАНД ОТ UI (FCDisplay)
    // ========================================================================

    /**
     * @brief Запустить операцию нанесения (слот для сигнала play() от UI)
     * @param container Контейнер с данными для печати
     * @note Асинхронный: команда ставится в очередь, выполнение — в рабочем потоке
     * @threadsafe Может вызываться из любого потока (UI или другого)
     * @see result(), progress()
     */
    void start(const FCSVGImageContainer &container);

    /**
     * @brief Экстренная остановка (слот для сигнала stop() от UI)
     * @note Немедленная: вызывает _hardware.emergencyStop()
     * @threadsafe Может вызываться из любого потока
     * @see start(), pause()
     */
    void stop();

    /**
     * @brief Приостановить операцию (слот для сигнала pause() от UI)
     * @note Для возобновления вызовите start() с тем же контейнером
     * @threadsafe Может вызываться из любого потока
     */
    void pause();

    /**
     * @brief Сбросить состояние плоттера (слот для сигнала reset() от UI)
     * @note Требует повторной инициализации перед новой операцией
     * @threadsafe Может вызываться из любого потока
     */
    void reset();

    /**
     * @brief Очистить печатающую головку (слот для сигнала clear() от UI)
     * @note Расходует чернила — используйте при смене цвета
     * @threadsafe Может вызываться из любого потока
     */
    void clear();

    /**
     * @brief Запустить короткий тест (слот для сигнала shortTest() от UI)
     * @note ~3 секунды, результат в сигнале test()
     * @threadsafe Может вызываться из любого потока
     */
    void shortTest();

    /**
     * @brief Запустить расширенный тест (слот для сигнала longTest() от UI)
     * @note ~30 секунд, результат в сигнале test()
     * @threadsafe Может вызываться из любого потока
     */
    void longTest();

signals:
    // ========================================================================
    // СИГНАЛЫ ОБРАТНОЙ СВЯЗИ ДЛЯ UI (FCDisplay)
    // ========================================================================

    /// @brief Операция успешно запущена
    /// @param name Имя источника (serialNumber())
    void started(const QString &name);

    /// @brief Операция успешно остановлена
    /// @param name Имя источника
    void stopped(const QString &name);

    /// @brief Операция приостановлена
    /// @param name Имя источника
    void paused(const QString &name);

    /// @brief Сброс завершён
    /// @param name Имя источника
    void resetCompleted(const QString &name);

    /// @brief Очистка завершена
    /// @param name Имя источника
    void cleared(const QString &name);

    /// @brief Информационное сообщение для лога/статуса
    /// @param name Источник, @param text Текст сообщения
    void message(const QString &name, const QString &text);

    /// @brief Ошибка в работе плоттера
    /// @param name Источник, @param text Описание ошибки
    void error(const QString &name, const QString &text);

    /// @brief Обновление прогресса выполнения
    /// @param name Источник, @param percent [0;100], @param layer номер слоя
    void progress(const QString &name, int percent, int layer);

    /// @brief Результат операции (успех/неудача)
    /// @param name Имя операции, @param success результат, @param details подробности
    void result(const QString &name, bool success, const QString &details);

    /// @brief Результат диагностического теста
    /// @param testName "short" или "long", @param success результат, @param details отчёт
    void test(const QString &testName, bool success, const QString &details);

    /// @brief Изменение готовности оборудования (проброс от FCPlotterHardware)
    /// @param ready true если оборудование инициализировано
    void hardwareReady(bool ready);

private:
    // --- Виртуальные методы инициализации (из FCDevice) ---
    bool init();
    bool final();

    // --- Приватные слоты: выполнение команд в рабочем потоке ---
    void run();  // Главный цикл обработки (вызывается через QThread::started)

    // --- Обработчики команд (выполняются в рабочем потоке) ---
    void processStartCommand();
    void processStopCommand();
    void processPauseCommand();
    void processResetCommand();
    void processClearCommand();
    void processShortTestCommand();
    void processLongTestCommand();

    // --- Вспомогательные методы ---
//    void connectHardwareSignals();
    [[nodiscard]] int estimateCurrentLayer(int cmdIdx, int totalCmds, int totalLayers) const;

    // --- Члены данных ---
    QString _serialNumber;                    ///< Уникальный идентификатор экземпляра
    FCPlotterHardware _hardware;              ///< Компонент управления оборудованием
    QThread *_workerThread = nullptr;         ///< Рабочий поток выполнения
    FCSVGImageContainer _currentContainer;    ///< Текущие данные для печати
    bool _stopRequested = false;              ///< Флаг запроса завершения

    // --- Константы ---
    static constexpr int PROGRESS_UPDATE_MS = 500;   ///< Интервал обновления прогресса
    static constexpr int CLEAR_DURATION_MS = 2000;   ///< Длительность очистки головки
};

using FCPlotterPtrList = QList<FCPlotter*>;

#endif // FC_PLOTTER_H
