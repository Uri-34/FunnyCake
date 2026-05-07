#ifndef FC_SCANER_H
#define FC_SCANER_H

#include "FCDevice.h"
#include "FCI2CBus.h"

class FCScaner
    : protected FCDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCScaner)
public:
    /// @brief Таймаут ожидания завершения рабочего потока (мс)
    static constexpr int THREAD_STOP_TIMEOUT_MS = 5000;

    // --- Конструкторы и деструктор ---
    explicit FCScaner(QString &portName, FCI2CBus *bus, QObject *parent = nullptr);
    ~FCScaner() override;

    // --- Управление потоком ---
    bool startThread();
    bool stopThread();

    inline bool readiness() override { return state().is(FCOpenState::Open, FCReadyState::Ready) && isThreadRunning(); }

public slots:
    void onStart(const QString &model);
    void onStop();
    void onPause();
    void onReset();

signals:
    void started(const QString &text, QObject *object = nullptr);
    void stoped(const QString &text, QObject *object = nullptr);
    void paused(const QString &text, QObject *object = nullptr);
    void reseted(const QString &text, QObject *object = nullptr);
    void message(const QString &text, QObject *object = nullptr);
    void error(const QString &text, QObject *object = nullptr);
    void progress(int percent, QObject *object = nullptr);

private:
    // виртуальные методы инициализации (из FCDevice) ---
    bool init();
    bool final();

    // проверка: запущен ли поток
    bool isThreadRunning() const noexcept;

    void run();  // Главный цикл обработки (вызывается через QThread::started)

    // --- Обработчики команд (выполняются в рабочем потоке) ---
    void processStart();
    void processStop();
    void processTest();

    QThread *_workerThread = nullptr; ///< Рабочий поток выполнения
};

#endif // FC_SCANER_H
