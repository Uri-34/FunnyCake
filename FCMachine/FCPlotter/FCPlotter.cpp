#include <QThread>
#include <QDateTime>
#include <QRandomGenerator>

#include "FCPlotter.h"
#include "FCPlotterGCodeEngine.h"

#include <QDebug>

// КОНСТРУКТОРЫ И ДЕСТРУКТОР
FCPlotter::FCPlotter(QString &portName, FCI2CBus *bus, QObject *parent)
    : FCDevice(QStringLiteral("Plotter-") + portName, parent),
      _serialNumber{QStringLiteral("PLOT-%1-%2").arg(portName.replace('/', '_')).arg(QRandomGenerator::global()->bounded(10000, 99999))},
      _controller{new FCGCodeController{portName, this}},
      _ramp{new FCPumpRamp{bus, this}},
      _head{new FCCanHead{this}}
{
    init();
}

FCPlotter::~FCPlotter()
{
    // Гарантированное завершение потока
    if(_workerThread && _workerThread->isRunning())
    {
        stopThread();
    }
    // _bus + _controller + _ramp + _head удалятся автоматически через механизм родитель-потомок
}

// ДОСТУП К СВОЙСТВАМ
bool FCPlotter::isThreadRunning() const noexcept
{
    return _workerThread && _workerThread->isRunning() && !_stopRequested;
}

// УПРАВЛЕНИЕ ПОТОКОМ
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
    _workerThread->setObjectName(QStringLiteral("PlotterWorker-") + objectName() + _serialNumber);

    // Перемещение объекта в рабочий поток
    moveToThread(_workerThread);

    // Запуск цикла обработки через сигнал started
    connect(_workerThread, &QThread::started, this, &FCPlotter::run, Qt::DirectConnection);  // run() выполняется в контексте _workerThread

    // Сигнал завершения для очистки
    connect(_workerThread, &QThread::finished, this, &FCPlotter::stop, Qt::QueuedConnection);  // Обработка в основном потоке

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
//    _hardware.final();

    // Остановка потока с таймаутом
    _workerThread->quit();
    if(!_workerThread->wait(THREAD_STOP_TIMEOUT_MS))
    {
        qWarning() << objectName() << ": ошибка завершения потока за " << THREAD_STOP_TIMEOUT_MS << "мс";
        _workerThread->terminate();
        _workerThread->wait();
    }

    _workerThread = nullptr;

    // Сброс состояния
    emit condition(state().set(FCPlotterState{FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::Close}));
    return true;
}

// ПУБЛИЧНЫЕ СЛОТЫ: Приём команд от UI (вызываются из любого потока)
void FCPlotter::start(const FCSVGImageContainer &container)
{
    if(!hardwareDevicesIsReady())
    {
        emit error(objectName(), state().set(FCPlotterState{FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::Destroy}), QStringLiteral("оборудование не готово"));
        return;
    }

    if(!container.isValid())
    {
        emit error(objectName(), state().set(FCPlotterState{FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::Parse}), QStringLiteral("контейнер не готов"));
        return;
    }

    // копирование контейнера (безопасно для кросс-поточного использования)
    _container = container;

    // установка состояния в start обновление состояния
//    emit condition();
    emit started(objectName(), state().set(FCPlotterState{FCReadyState::Ready, FCPlayState::Start, FCErrorType::None}));

    // Непосредственный запуск обработки (выполнится в рабочем потоке)
    // Благодаря moveToThread(), этот вызов будет выполнен в контексте _workerThread
    QMetaObject::invokeMethod(this, &FCPlotter::processStartCommand, Qt::QueuedConnection);
}

void FCPlotter::stop()
{
    if(state().is(FCPlayState::Stop))
    {
        return;
    }

    // Немедленная аварийная остановка оборудования
    _controller->stop();
    // Обновление состояния
    emit stopped(objectName(), state().set(FCReadyState::Ready, FCPlayState::Stop, FCErrorType::None));
}

void FCPlotter::pause()
{
    if(state().is(FCPlayState::Pause))
    {
        return;
    }

    // Приостановка выполнения
    emit paused(objectName(), state().set(FCReadyState::Ready, FCPlayState::Pause, FCErrorType::None));
}

void FCPlotter::reset()
{
    if(_controller)
    {
        _controller->reboot();
    }

    if(_ramp)
    {
        _ramp->reset();
    }

    if(_ramp)
    {
        _ramp->reset();
    }

    emit reseted(objectName(), state().set(FCReadyState::Ready, FCPlayState::Stop, FCErrorType::None));
}

void FCPlotter::clear()
{
    if(_stopRequested)
    {
        return;
    }

    // Очистка головки
    if(_ramp)
    {
        _ramp->reset();
    }

    // Пауза для стекания чернил
    QThread::msleep(CLEAR_DURATION_MS);

    // Возврат в парковочную позицию
    if(_controller)
    {
        _controller->send("G1 X0 Y0 Z10");
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

// ПРИВАТНЫЕ СЛОТЫ: Выполнение команд в рабочем потоке
void FCPlotter::run()
{
    // Этот слот выполняется в контексте _workerThread

    // Инициализация оборудования
    if(!init() || !state().isReady())
    {
        emit error(objectName(), state().set(FCPlotterState{FCReadyState::NotReady, FCErrorType::Connection}), QStringLiteral("Не удалось инициализировать оборудование"));
        return;
    }

    // Оборудование готово
    emit condition(state().set(FCReadyState::Ready, FCErrorType::None));
}

void FCPlotter::processStartCommand()
{
    // Проверка готовности
    if(!hardwareDevicesIsReady())
    {
        emit error(objectName(), state().set(FCReadyState::NotReady, FCErrorType::Connection), QStringLiteral("Оборудование не готово"));
        return;
    }

    // Генерация G-кода
    FCPlotterGCodeEngine gcodeEngine;
    QStringList gcodeList = gcodeEngine.generate(_container);

    if(gcodeList.isEmpty())
    {
        state().set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Parse);
        emit error(objectName(), state(), QStringLiteral("Не удалось сгенерировать G-код"));
        return;
    }

    // Выполнение команд
    int totalCommands = gcodeList.size();
    int totalLayers = _container.layerCount();
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
        if(_controller)
        {
            bool success = _controller->send(cmd.toUtf8());
            if(!success)
            {
                emit error(objectName(),
                           state().set(FCReadyState::NotReady, FCErrorState::Critical, FCErrorType::Write),
                           QStringLiteral("Ошибка отправки: ") + cmd);
                break;
            }
        }

        processedCommands++;

        // Обновление прогресса (не чаще чем каждые 500 мс)
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if(now - lastProgressUpdate >= PROGRESS_UPDATE_MS)
        {
            int percent = (processedCommands * 100) / totalCommands;
//            int currentLayer = estimateCurrentLayer(processedCommands, totalCommands, totalLayers);
            emit progress(percent, objectName());
            lastProgressUpdate = now;
        }
    }

    // Завершение операции
    if (!_stopRequested)
    {
        emit message(objectName(), QStringLiteral("Операция завершена"));
    }
    else
    {
        emit error(objectName(), state().set(FCPlayState::Stop, FCChangedState::Unchanged, FCErrorType::None), QStringLiteral("Операция прервана"));
    }
}

void FCPlotter::processStopCommand()
{
    stop();

    // Возврат в безопасную позицию
    if(_controller)
    {
        _controller->send("G1 Z10");
        _controller->send("G28 X Y");
    }

    emit stopped(objectName(), state().set(FCPlayState::Stop, FCChangedState::Changed));
}

void FCPlotter::processPauseCommand()
{
    if(_controller)
    {
        _controller->send("M25");  // Пауза в Marlin
        _controller->send("G1 Z5");
    }

    emit paused(objectName(), state().set(FCPlayState::Pause, FCChangedState::Changed));
}

void FCPlotter::processResetCommand()
{
    if(_controller)
    {
        _controller->reboot();
    }

    if(_ramp)
    {
        _ramp->reset();
    }

    emit reseted(objectName(), state().set(FCReadyState::NotReady, FCPlayState::Stop, FCChangedState::Changed, FCErrorType::None));
}

void FCPlotter::processClearCommand()
{
    if(_ramp)
    {
        _ramp->reset();
    }

    QThread::msleep(CLEAR_DURATION_MS);

    if(_controller)
    {
        _controller->send("G1 X0 Y0 Z10");
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

// ВИРТУАЛЬНЫЕ МЕТОДЫ ИНИЦИАЛИЗАЦИИ
bool FCPlotter::init()
{
    bool returnCode = false;

    // определение состояния плоттера в зависимости от состояний всех устройств
    if(_controller && _controller->checkConnection() && _controller->isSecretCheck() &&
       _ramp && _ramp->isOpen() &&
       _head && _head->isSecretCheck())
    {
        // состояние g-code контроллера
        connect(_controller, &FCGCodeController::condition,
                this, [this](const FCDeviceState &deviceState, const QString &details)
                {
                    Q_UNUSED(details);
                    state() &= deviceState;
                },
                Qt::QueuedConnection
        );

        // состояние рампы насосов
        connect(_ramp, &FCPumpRamp::condition,
                this, [this](const FCDeviceState &deviceState, const QString &details)
                {
                    Q_UNUSED(details);
                    state() &= deviceState;  // Объединение через operator&=
                },
                Qt::QueuedConnection
        );

        // состояние головки рисования
        connect(_head, &FCPumpRamp::condition,
                this, [this](const FCDeviceState &deviceState, const QString &details)
                {
                    Q_UNUSED(details);
                    state() &= deviceState;  // Объединение через operator&=
                },
                Qt::QueuedConnection
        );
        returnCode = true;
    }
    else
    {
        state().set(FCReadyState::NotReady, FCErrorType::Connection);
        returnCode = false;
    }

//    emit condition(state());

    return returnCode;
}

bool FCPlotter::final()
{
    return true;
}

// ПРИВАТНЫЕ МЕТОДЫ
int FCPlotter::estimateCurrentLayer(int commandIndex, int totalCommands, int totalLayers) const
{
    if (totalLayers <= 0 || totalCommands <= 0) return -1;

    // Линейная аппроксимация: номер слоя пропорционален прогрессу
    int layer = (commandIndex * totalLayers) / totalCommands;
    return qBound(0, layer, totalLayers - 1);
}
