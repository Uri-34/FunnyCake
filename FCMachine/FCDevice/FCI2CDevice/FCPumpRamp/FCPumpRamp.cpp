#include "FCPumpRamp.h"
#include <QtMath>

// Конструктор / Деструктор
FCPumpRamp::FCPumpRamp(FCI2CBus *bus, QObject *parent)
    : FCI2CDevice(bus, 0x30, "Рампа насосов", parent),
      _pumps{1, 2, 3},
      _tempPollTimer(this)
{
    init();
}

FCPumpRamp::~FCPumpRamp()
{
    final();
}

bool FCPumpRamp::init()
{
    // настройка таймера: интервал опроса 1000 мс (1 секунда)
    _tempPollTimer.setInterval(1000);
    _tempPollTimer.setSingleShot(false);

    connect(&_tempPollTimer, &QTimer::timeout, this, [this]() { onTemperaturePoll(); });

    // первичный опрос температур (чтобы сразу иметь актуальные данные)
    onTemperaturePoll();

    return true;
}

bool FCPumpRamp::final()
{
    // Останавливаем таймер перед завершением работы устройства
    _tempPollTimer.stop();
    clear();

    return true;
}

// Публичные методы
qreal FCPumpRamp::thermometer(int number) const
{
    // здесь должна быть реальная логика чтения с датчика LM75A
    Q_UNUSED(number);
    return 25.0;  // Возвращаем фиксированное значение для примера
}

bool FCPumpRamp::switchTo(uint8_t pumpNumber)
{
    // Гарантируем: сначала выключаем все насосы
    clear();

    // Включаем целевой насос (реализация зависит от вашего протокола)
    // Пример: отправка команды включения насоса #pumpNumber
    if(!send({static_cast<char>(pumpNumber), static_cast<char>(0x01)}))
    {
        return false;
    }

    emit pumpSwitched(pumpNumber);
    return true;
}

void FCPumpRamp::onStart(const QString &model)
{
    Q_UNUSED(model);
    clear();

    _tempPollTimer.start();
    condition(state().set(FCPlayState::Start), this);
}

void FCPumpRamp::onStop()
{
    clear();
    condition(state().set(FCPlayState::Stop), this);
}

void FCPumpRamp::onPause()
{
    // Логика паузы (например, приостановка опроса)
    _tempPollTimer.stop();

    condition(state().set(FCPlayState::Pause), this);
}

void FCPumpRamp::onTest()
{
    // Тестовый режим: принудительный опрос и вывод температур
    onTemperaturePoll();

    // пока не решил - наверное нужно попереключать двигатели ! а может ничего делать и не нужно !!! :)
}

// Private Slots: Обработчики
void FCPumpRamp::onTemperaturePoll()
{
    QList<float> current;
    current.reserve(_pumps.size());

    // Считываем температуры со всех подключённых датчиков
    for(uint8_t pumpNum : _pumps)
    {
        current.append(thermometer(pumpNum));
    }

    // Эмитим сигнал ТОЛЬКО если данные реально изменились
    // Это защищает от лишней нагрузки на слоты-получатели и сеть
    if (current != _lastTemperatures)
    {
        _lastTemperatures = current;
        // Прямой вызов сигнала (современный Qt5/6 стиль, без 'emit')
        emit temperatureChanged(current);
    }
}
