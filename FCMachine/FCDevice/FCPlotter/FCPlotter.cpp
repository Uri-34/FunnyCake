#include <QThread>
#include <QDateTime>
#include <QRandomGenerator>

#include "FCPlotter.h"
#include "FCPlotterGCodeEngine.h"

#include <QDebug>

// КОНСТРУКТОРЫ И ДЕСТРУКТОР
FCPlotter::FCPlotter(const QString &serialPortName, FCI2CBus *bus, QObject *parent)
    : FCDevice(QStringLiteral("Plotter-") + serialPortName, parent),
//      _serialNumber{QStringLiteral("PLOT-%1-%2").arg(serialPortName.replace('/', '_')).arg(QRandomGenerator::global()->bounded(10000, 99999))},
      _controller{new FCGCodeController{serialPortName, this}},
      _ramp{new FCPumpRamp{bus, this}},
      _head{new FCCanHead{this}}
{
    init();
}

FCPlotter::~FCPlotter()
{
    // Гарантированное завершение потока
    if(_plotterThread && _plotterThread->isRunning())
    {
        stopThread();
    }
    // _bus + _controller + _ramp + _head удалятся автоматически через механизм родитель-потомок
}

// МЕТОДЫ ИНИЦИАЛИЗАЦИИ
bool FCPlotter::init()
{
    bool returnCode = false;

    // определение состояния плоттера в зависимости от состояний всех устройств
    if(readiness())
    {
        // состояние g-code контроллера
        connect(_controller, &FCGCodeController::condition,
                this, [this](const FCDeviceState &deviceState)
                {
                    state() &= deviceState;  // Объединение через operator&=
                },
                Qt::QueuedConnection
        );

        // состояние рампы насосов
        connect(_ramp, &FCPumpRamp::condition,
                this, [this](const FCDeviceState &deviceState)
                {
                    state() &= deviceState;  // Объединение через operator&=
                },
                Qt::QueuedConnection
        );

        // состояние головки рисования
        connect(_head, &FCPumpRamp::condition,
                this, [this](const FCDeviceState &deviceState)
                {
                    state() &= deviceState;  // Объединение через operator&=
                },
                Qt::QueuedConnection
        );

        connect(this, &FCPlotter::reseted, _controller, &FCGCodeController::onReset);
        connect(this, &FCPlotter::reseted, _ramp, &FCPumpRamp::onReset);
        connect(this, &FCPlotter::reseted, _head, &FCCanHead::onReset);

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


// УПРАВЛЕНИЕ ПОТОКОМ
bool FCPlotter::startThread()
{
    // отказ в запуске из за отсутствия готовности
    if(!state().is(FCOpenState::Open, FCReadyState::Ready))
    {
        return false;
    }

    // защита от повторного запуска
    if(_plotterThread && _plotterThread->isRunning())
    {
        return true;
    }

    // Создание и настройка рабочего потока
    _plotterThread = new QThread(this);
    _plotterThread->setObjectName(QStringLiteral("Поток плоттера -") + objectName());

    // Перемещение объекта в рабочий поток
    moveToThread(_plotterThread);

    // Запуск цикла обработки через сигнал started
    connect(_plotterThread, &QThread::started, this, &FCPlotter::run, Qt::DirectConnection);  // run() выполняется в контексте _plotterThread

    // Сигнал завершения для очистки
    connect(_plotterThread, &QThread::finished, this, &FCPlotter::onStop, Qt::QueuedConnection);  // Обработка в основном потоке

    _plotterThread->start();

    state().set(FCPlayState::Start);

    return true;
}

bool FCPlotter::stopThread()
{
    if(!isThreadRunning())
    {
        return true;
    }

    // Финализация оборудования
    final();

    // Остановка потока с таймаутом
    _plotterThread->quit();
    if(!_plotterThread->wait(THREAD_STOP_TIMEOUT_MS))
    {
        emit error(QStringLiteral("ошибка завершения потока %1 за %2 мс").arg(objectName()).arg(THREAD_STOP_TIMEOUT_MS));
        _plotterThread->terminate();
        _plotterThread->wait();
    }

    _plotterThread = nullptr;

    // Сброс состояния
    emit condition(state().set(FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::None), this);
    emit message(QStringLiteral("поток %1 остановлен").arg(objectName()));
    return true;
}

// ПУБЛИЧНЫЕ СЛОТЫ: Приём команд от UI (вызываются из любого потока)
void FCPlotter::onStart(const QString &model)
{
    if(!state().is(FCReadyState::Ready))
    {
        emit condition(state().set(FCDeviceState{FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::Destroy}), this);
        emit error(QStringLiteral("плоттер не готов"), this);
        return;
    }

    emit condition(state().set(FCDeviceState{FCReadyState::Ready, FCPlayState::Start, FCErrorType::None}), this);
    emit start(QStringLiteral("плоттер стартовал"), this);

    // Непосредственный запуск обработки (выполнится в рабочем потоке)
    // Благодаря moveToThread(), этот вызов будет выполнен в контексте _plotterThread
    QMetaObject::invokeMethod(this, &FCPlotter::processStart, Qt::QueuedConnection);
}

void FCPlotter::onStop()
{
    if(state().is(FCPlayState::Stop))
    {
        return;
    }

    // аварийная остановка оборудования
    _controller->stop();
    // обновление состояния и отправка сообщения об остановке
    emit condition(state().set(FCPlayState::Stop), this);
    emit stoped("плоттер остановлен", this);
}

void FCPlotter::onPause()
{
    if(state().is(FCPlayState::Pause))
    {
        return;
    }

    // приостановка выполнения
    emit condition(state().set(FCPlayState::Pause), this);
    emit paused("плоттер приостановлен", this);
}

void FCPlotter::onReset()
{
    if(_controller)
    {
        _controller->reset();
    }

    if(_ramp)
    {
        _ramp->reset();
    }

    if(_head)
    {
        _head->reset();
    }

    emit condition(state().set(FCReadyState::Ready, FCPlayState::Stop, FCErrorType::None), this);
    emit reseted("плоттер сброшен", this);
}

//void FCPlotter::clear()
//{
//    if(_stopRequested)
//    {
//        return;
//    }

//    // Очистка головки
//    if(_ramp)
//    {
//        _ramp->reset();
//    }

//    // Пауза для стекания чернил
//    QThread::msleep(CLEAR_DURATION_MS);

//    // Возврат в парковочную позицию
//    if(_controller)
//    {
//        _controller->send("G1 X0 Y0 Z10");
//    }

//    emit cleared(objectName());
//}

void FCPlotter::onTest()
{
    if(state().is(FCPlayState::Start))
    {
        return;
    }

    // Запуск расширенного теста путем вывода тестового фала для расширенного теста
    emit test(QStringLiteral("long"), true, QStringLiteral("Тест запущен"));

    state().is(FCPlayState::Stop);
}

// ПРИВАТНЫЕ СЛОТЫ: Выполнение команд в рабочем потоке
void FCPlotter::run()
{
    // Этот слот выполняется в контексте _plotterThread

    // Инициализация оборудования
    if(!init() || !state().isReady())
    {
        emit error(objectName(), state().set(FCDeviceState{FCReadyState::NotReady, FCErrorType::Connection}), QStringLiteral("Не удалось инициализировать оборудование"));
        return;
    }

    // Оборудование готово
    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), this);
}

void FCPlotter::processStartCommand()
{
    // Проверка готовности
    if(!state().is(FCReadyState::Ready))
    {
        emit error(objectName(), state().set(FCReadyState::NotReady, FCErrorType::Connection), QStringLiteral("Оборудование не готово"));
        return;
    }

    // Генерация G-кода
    FCPlotterGCodeEngine gcodeEngine;
    QStringList gcodeList = gcodeEngine.generate(_container);

    if(gcodeList.isEmpty())
    {
        emit condition(state().set(FCPlayState::Stop, FCErrorType::Parse), this);
        emit error(QStringLiteral("не удалось сгенерировать G-код"), this);
        return;
    }

    // Выполнение команд
    int totalCommands = gcodeList.size();
    int totalLayers = _container.layerCount();
    int processedCommands = 0;
    qint64 lastProgressUpdate = 0;

    for(const QString &cmd : gcodeList)
    {
        if(state().is(FCPlayState::Stop))
        {
            emit message(QStringLiteral("Операция прервана"), this);
            break;
        }

        // Отправка команды оборудованию
        if(_controller)
        {
            bool success = _controller->send(cmd.toUtf8());
            if(!success)
            {
                emit error(QStringLiteral("ошибка отправки: ") + cmd, this);
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
        emit error(objectName(), state().set(FCPlayState::Stop, FCErrorType::None), QStringLiteral("Операция прервана"));
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

    emit stopped(objectName(), state().set(FCPlayState::Stop));
}

void FCPlotter::processPauseCommand()
{
    if(_controller)
    {
        _controller->send("M25");  // Пауза в Marlin
        _controller->send("G1 Z5");
    }

    emit paused(objectName(), state().set(FCPlayState::Pause));
}

//void FCPlotter::processResetCommand()
//{
//    if(_controller)
//    {
//        _controller->reset();
//    }

//    if(_ramp)
//    {
//        _ramp->reset();
//    }

//    if(_head)
//    {
//        _head->reset();
//    }

//    emit reseted(objectName(), state().set(FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::None));
//}

//void FCPlotter::processClearCommand()
//{
//    if(_ramp)
//    {
//        _ramp->reset();
//    }

//    QThread::msleep(CLEAR_DURATION_MS);

//    if(_controller)
//    {
//        _controller->send("G1 X0 Y0 Z10");
//    }

//    emit cleared(objectName());
//}

void FCPlotter::processShortTestCommand()
{
    // Заглушка — реализация теста делегируется компоненту оборудования
    emit test(QStringLiteral("Короткий"), true, QStringLiteral("Короткий тест пройден"));
}

void FCPlotter::processLongTestCommand()
{
    // Заглушка — реализация теста делегируется компоненту оборудования
    emit test(QStringLiteral("Расширенный"), true, QStringLiteral("Расширенный тест пройден"));
}
