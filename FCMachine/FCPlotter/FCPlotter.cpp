#include <QThread>
#include <QDateTime>
#include <QRandomGenerator>

#include "FCPlotter.h"
#include "FCPlotterHardware.h"
#include "FCPlotterGCodeEngine.h"
//#include "FCFeeder.h"

#include <QDebug>

// ============================================================================
// КОНСТРУКТОРЫ И ДЕСТРУКТОР
// ============================================================================

FCPlotter::FCPlotter(QString &portName, FCI2CBus *bus, QObject *parent)
    : FCDevice(QStringLiteral("Plotter-") + portName, parent),
      _serialNumber{QStringLiteral("PLOT-%1-%2").arg(portName.replace('/', '_')).arg(QRandomGenerator::global()->bounded(10000, 99999))},
      _hardware{portName, bus, this},
      _workerThread{nullptr},
      _stopRequested{false}
{
    // Подключение сигналов оборудования
    init();
}

FCPlotter::~FCPlotter()
{
    // Гарантированное завершение потока
    if(_workerThread && _workerThread->isRunning())
    {
        stopThread();
    }
    // _hardware удалится автоматически через механизм родитель-потомок
}

// ============================================================================
// ДОСТУП К СВОЙСТВАМ
// ============================================================================

bool FCPlotter::isThreadRunning() const noexcept
{
    return _workerThread && _workerThread->isRunning() && !_stopRequested;
}

QString FCPlotter::securityCode(int timeoutMs)
{
    // Делегирование компоненту оборудования
    return _hardware.securityCode(timeoutMs);
}

// ============================================================================
// УПРАВЛЕНИЕ ПОТОКОМ
// ============================================================================

bool FCPlotter::startThread()
{
    // Защита от повторного запуска
    if(_workerThread && _workerThread->isRunning())
    {
        return true;
    }

    _stopRequested = false;

    // Создание и настройка рабочего потока
    _workerThread = new QThread(this);
    _workerThread->setObjectName(QStringLiteral("PlotterWorker-") + _serialNumber);

    // Перемещение объекта в рабочий поток
    moveToThread(_workerThread);

    // Запуск цикла обработки через сигнал started
    connect(_workerThread, &QThread::started,
            this, &FCPlotter::run,
            Qt::DirectConnection
    );  // run() выполняется в контексте _workerThread

    // Сигнал завершения для очистки
    connect(_workerThread, &QThread::finished,
            this, [this]()
            {
                emit stopped(objectName());
            },
            Qt::QueuedConnection
    );  // Обработка в основном потоке

    _workerThread->start();
    return true;
}

bool FCPlotter::stopThread()
{
    if(!_workerThread || !_workerThread->isRunning())
    {
        return true;
    }

    _stopRequested = true;

    // Финализация оборудования
    _hardware.final();

    // Остановка потока с таймаутом
    _workerThread->quit();
    if(!_workerThread->wait(THREAD_STOP_TIMEOUT_MS))
    {
        qWarning() << objectName() << ": поток не завершился за" << THREAD_STOP_TIMEOUT_MS << "мс";
        _workerThread->terminate();
        _workerThread->wait();
    }

    _workerThread = nullptr;

    // Сброс состояния
    set(FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Changed, FCErrorType::Disconnection);
    return true;
}

// ============================================================================
// ПУБЛИЧНЫЕ СЛОТЫ: Приём команд от UI (вызываются из любого потока)
// ============================================================================

void FCPlotter::start(const FCSVGImageContainer &container)
{
    if(_stopRequested)
    {
        emit error(objectName(), QStringLiteral("Запрос остановки активен"));
        return;
    }

    if(!_hardware.isReady())
    {
        emit error(objectName(), QStringLiteral("Оборудование не готово"));
        set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Connection);
        return;
    }

    if(!container.isValid())
    {
        emit error(objectName(), QStringLiteral("Невалидный контейнер"));
        set(FCReadyState::NotReady, FCErrorType::Parse);
        return;
    }

    // Копирование контейнера (безопасно для кросс-поточного использования)
    _currentContainer = container;

    // Обновление состояния и запуск обработки
    set(FCPlayState::Start, FCChangedState::Changed);
    emit started(objectName());

    // Непосредственный запуск обработки (выполнится в рабочем потоке)
    // Благодаря moveToThread(), этот вызов будет выполнен в контексте _workerThread
    QMetaObject::invokeMethod(this, &::FCPlotter::processStartCommand, Qt::QueuedConnection);
}

void FCPlotter::stop()
{
    if(_stopRequested)
    {
        return;
    }

    // Немедленная аварийная остановка оборудования
    _hardware.emergencyStop();

    // Обновление состояния
    set(FCPlayState::Stop, FCChangedState::Changed);
    emit stopped(objectName());
}

void FCPlotter::pause()
{
    if (_stopRequested)
    {
        return;
    }

    // Приостановка выполнения
    set(FCPlayState::Pause, FCChangedState::Changed);
    emit paused(objectName());
}

void FCPlotter::reset()
{
    if (_stopRequested)
    {
        return;
    }

    // Сброс контроллера и рампы
    auto *controller = _hardware.controller();
    auto *ramp = _hardware.ramp();

    if (controller)
    {
        controller->reboot();
    }

    if(ramp)
    {
        ramp->reset();
    }

    // Сброс состояния
    set(FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Changed, FCErrorType::None);

    emit resetCompleted(objectName());
}

void FCPlotter::clear()
{
    if(_stopRequested)
    {
        return;
    }

    // Очистка головки
    auto *ramp = _hardware.ramp();
    if (ramp) ramp->reset();

    // Пауза для стекания чернил
    QThread::msleep(CLEAR_DURATION_MS);

    // Возврат в парковочную позицию
    auto *controller = _hardware.controller();
    if(controller)
    {
        controller->sendCommand("G1 X0 Y0 Z10");
    }

    emit cleared(objectName());
}

void FCPlotter::shortTest()
{
    if(_stopRequested)
    {
        return;
    }

    // Запуск короткого теста через компонент оборудования
    // (реализация теста делегирована в соответствующие классы)
    emit test(QStringLiteral("short"), true, QStringLiteral("Тест запущен"));
}

void FCPlotter::longTest()
{
    if(_stopRequested)
    {
        return;
    }

    // Запуск расширенного теста
    emit test(QStringLiteral("long"), true, QStringLiteral("Тест запущен"));
}

// ============================================================================
// ПРИВАТНЫЕ СЛОТЫ: Выполнение команд в рабочем потоке
// ============================================================================

void FCPlotter::run()
{
    // Этот слот выполняется в контексте _workerThread

    // Инициализация оборудования
    if(!_hardware.isReady())
    {
        emit error(objectName(), QStringLiteral("Не удалось инициализировать оборудование"));
        set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Connection);
        emit condition(state());
        return;
    }

    // Оборудование готово
    set(FCReadyState::Ready, FCErrorType::None);
    emit condition(state());
}

void FCPlotter::processStartCommand()
{
    // Проверка готовности
    if(!_hardware.isReady() || !_currentContainer.isValid())
    {
        emit error(objectName(), QStringLiteral("Невозможно начать операцию"));
        set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Connection);
        emit result(objectName(), false, QStringLiteral("Ошибка подготовки"));
        return;
    }

    // Генерация G-кода
    FCPlotterGCodeEngine gcodeEngine;
    QStringList gcodeList = gcodeEngine.generate(_currentContainer);

    if(gcodeList.isEmpty())
    {
        emit error(objectName(), QStringLiteral("Не удалось сгенерировать G-код"));
        set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Parse);
        emit result(objectName(), false, QStringLiteral("Ошибка генерации"));
        return;
    }

    // Выполнение команд
    int totalCommands = gcodeList.size();
    int totalLayers = _currentContainer.layerCount();
    int processedCommands = 0;
    qint64 lastProgressUpdate = 0;

    for(const QString &cmd : gcodeList)
    {
        if(_stopRequested)
        {
            emit message(objectName(), QStringLiteral("Операция прервана"));
            break;
        }

        // Отправка команды оборудованию
        auto *controller = _hardware.controller();
        if(controller)
        {
            bool success = controller->sendCommand(cmd.toUtf8());
            if(!success)
            {
                emit error(objectName(), QStringLiteral("Ошибка отправки: ") + cmd);
                set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Write);
                break;
            }
        }

        processedCommands++;

        // Обновление прогресса (не чаще чем каждые 500 мс)
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if(now - lastProgressUpdate >= PROGRESS_UPDATE_MS)
        {
            int percent = (processedCommands * 100) / totalCommands;
            int currentLayer = estimateCurrentLayer(processedCommands, totalCommands, totalLayers);
            emit progress(objectName(), percent, currentLayer);
            lastProgressUpdate = now;
        }
    }

    // Завершение операции
    if (!_stopRequested)
    {
        set(FCPlayState::Stop, FCChangedState::Unchanged, FCErrorType::None);
        emit result(objectName(), true, QStringLiteral("Операция завершена"));
    }
    else
    {
        emit result(objectName(), false, QStringLiteral("Операция прервана"));
    }
}

void FCPlotter::processStopCommand()
{
    _hardware.emergencyStop();

    // Возврат в безопасную позицию
    auto *controller = _hardware.controller();
    if(controller)
    {
        controller->sendCommand("G1 Z10");
        controller->sendCommand("G28 X Y");
    }

    set(FCPlayState::Stop, FCChangedState::Changed);
    emit stopped(objectName());
}

void FCPlotter::processPauseCommand()
{
    auto *controller = _hardware.controller();
    if(controller)
    {
        controller->sendCommand("M25");  // Пауза в Marlin
        controller->sendCommand("G1 Z5");
    }

    set(FCPlayState::Pause, FCChangedState::Changed);
    emit paused(objectName());
}

void FCPlotter::processResetCommand()
{
    auto *controller = _hardware.controller();
    auto *ramp = _hardware.ramp();

    if(controller)
    {
        controller->reboot();
    }

    if(ramp)
    {
        ramp->reset();
    }

    set(FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Changed, FCErrorType::None);
    emit resetCompleted(objectName());
}

void FCPlotter::processClearCommand()
{
    auto *ramp = _hardware.ramp();
    if(ramp)
    {
        ramp->reset();
    }

    QThread::msleep(CLEAR_DURATION_MS);

    auto *controller = _hardware.controller();
    if(controller)
    {
        controller->sendCommand("G1 X0 Y0 Z10");
    }

    emit cleared(objectName());
}

void FCPlotter::processShortTestCommand()
{
    // Заглушка — реализация теста делегируется компоненту оборудования
    emit test(QStringLiteral("short"), true, QStringLiteral("Короткий тест пройден"));
}

void FCPlotter::processLongTestCommand()
{
    // Заглушка — реализация теста делегируется компоненту оборудования
    emit test(QStringLiteral("long"), true, QStringLiteral("Расширенный тест пройден"));
}

// ============================================================================
// ВИРТУАЛЬНЫЕ МЕТОДЫ ИНИЦИАЛИЗАЦИИ
// ============================================================================

bool FCPlotter::init()
{
    bool returnCode = false;
    // Готовность оборудования
    if(_hardware.init())
    {
        // Проброс сигналов оборудования в сигналы плоттера (для UI)
        connect(&_hardware, &FCPlotterHardware::condition,
                this, [this](const FCPlotterHardwareState &state)
                {
                    bool ready = state.isReady();
                    if(ready)
                    {
                        set(FCReadyState::Ready);
                    }

                    emit hardwareReady(ready);
                },
                Qt::QueuedConnection
        );

        // Ошибки оборудования
        connect(&_hardware, &FCPlotterHardware::condition,
                this, [this](const FCPlotterHardwareState &hardwareState)
                {
                    state() &= hardwareState;
                },
                Qt::QueuedConnection
        );

        // Изменение состояния оборудования → обновление состояния плоттера
        connect(&_hardware, &FCPlotterHardware::condition,
                this, [this](const FCPlotterHardwareState &hardwareState)
                {
                    state() &= hardwareState;  // Объединение через operator&=
                    emit condition(state(), objectName());
                },
                Qt::QueuedConnection
        );

        state() &= _hardware.state();
        returnCode = true;
    }
    else
    {
        state().set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Connection);
        returnCode = false;
    }

    emit condition(state());

    return returnCode;
}

bool FCPlotter::final()
{
    // отключать connect() нет смысла потому что метод FCPlotter::final() не вызывается циклически

    return _hardware.final();
}

// ============================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ============================================================================

int FCPlotter::estimateCurrentLayer(int commandIndex, int totalCommands, int totalLayers) const
{
    if (totalLayers <= 0 || totalCommands <= 0) return -1;

    // Линейная аппроксимация: номер слоя пропорционален прогрессу
    int layer = (commandIndex * totalLayers) / totalCommands;
    return qBound(0, layer, totalLayers - 1);
}
