#ifndef FC_PLOTTER_H
#define FC_PLOTTER_H

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QThread>

#include "FCDevice.h"
#include "FCI2CBus.h"
#include "FCSVGImageParser.h"
#include "FCImageBinaryContainer.h"
#include "FCGCodeController.h"
#include "FCPumpRamp.h"
#include "FCCanHead.h"

class FCPlotter
    : protected FCDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCPlotter)
public:
    /// @brief Таймаут ожидания завершения рабочего потока (мс)
    static constexpr int THREAD_STOP_TIMEOUT_MS = 5000;

    explicit FCPlotter(QString &serialPortName, FCI2CBus *bus, QObject *parent = nullptr);
    ~FCPlotter() override;

    inline bool readiness() override
    {
        return _controller->readiness() && _ramp->readiness() && _head->readiness() &&
               _parcer.readiness() && _container.readiness() &&
                isThreadRunning() && isSecretCheck();
    }

    // управление потоком
    bool startThread();
    bool stopThread();

public slots:
    void onStart(const QString &model) override;
    void onStop() override;
    void onPause() override;
//    void onReset() override;
    void onTest() override;

private:
    bool init();
    bool final();

    inline bool isThreadRunning() noexcept { return _plotterThread && _plotterThread->isRunning(); }
    inline bool isSecretCheck() { return _controller->isSecretCheck() && _ramp->isSecretCheck() && _head->isSecretCheck(); }

    // --- Приватные слоты: выполнение команд в рабочем потоке ---
    void run();  // Главный цикл обработки (вызывается через QThread::started)

    // --- Обработчики команд (выполняются в рабочем потоке) ---
    void processStart();
    void processStop();
    void processPause();
    void processReset();
    void processTest();

    // --- Вспомогательные методы ---
    [[nodiscard]] int estimateCurrentLayer(int cmdIdx, int totalCmds, int totalLayers) const;

    // члены данных
    // сервисные обьекты
    FCSVGImageParser _parcer; ///< парсер svg файла
    FCImageBinaryContainer _container; ///< контейнер с данными для печати

    // обьекты устройств
    FCGCodeController *_controller = nullptr; ///< указатель на контроллер
    FCPumpRamp *_ramp = nullptr; ///< указатель на рампу насосов
    FCCanHead *_head = nullptr; ///< указатель на прокси головки

    // паралельный поток
    QThread *_plotterThread = nullptr; ///< Рабочий поток выполнения

    static constexpr int PROGRESS_UPDATE_MS = 500; ///< Интервал обновления прогресса
    static constexpr int CLEAR_DURATION_MS = 2000; ///< Длительность очистки головки
};

#endif // FC_PLOTTER_H
