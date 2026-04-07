#include "FCPumpRamp.h"

FCPumpRamp::FCPumpRamp(FCI2CBus *bus, QObject *parent)
: FCI2CDevice(bus, 0x20, "Ramp", parent),
  _pumps{
      {QColor(Qt::red), 0xFE},
      {QColor(Qt::green), 0xFD},
      {QColor(Qt::blue), 0xFB}
  }
{
    // init() вызывается автоматически
}

bool FCPumpRamp::init()
{
    if(!FCI2CDevice::init())
    {
        return false;
    }

    _thermometers.append(new FCLM75AThermometer(bus(), 0x30, "t0", this));
    _thermometers.append(new FCLM75AThermometer(bus(), 0x31, "t1", this));
    _thermometers.append(new FCLM75AThermometer(bus(), 0x32, "t2", this));

    if(!reset())
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
        return false;
    }

    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return true;
}

bool FCPumpRamp::final()
{
    _thermometers.clear();
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::None), objectName());
    return true;
}

bool FCPumpRamp::reset()
{
    QByteArray command;
    command.append(static_cast<char>(0xFF));
    command.append(static_cast<char>(0xFF));

    if(writeBytes(command))
    {
        emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
        return true;
    }
    else
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
        return false;
    }
}

bool FCPumpRamp::switchTo(const QColor &color)
{
    if(!reset())
    {
        return false;
    }

    uint8_t pumpNumber = selectPumpNumber(color);
    QByteArray command;
    command.append(static_cast<char>(_pumps.at(pumpNumber).second));
    command.append(static_cast<char>(0xFF));

    bool success = writeBytes(command);
    if(success)
    {
        emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
        emit pumpSwitched(pumpNumber);
    }
    else
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
    }
    return success;
}

uint8_t FCPumpRamp::selectPumpNumber(const QColor &color) const
{
    for(int i = 0; i < _pumps.size(); ++i)
    {
        if(_pumps.at(i).first == color)
        {
            return static_cast<uint8_t>(i);
        }
    }

    return 0;
}

FCLM75AThermometer* FCPumpRamp::thermometer(int index) const
{
    if(index >= 0 && index < _thermometers.size())
    {
        return _thermometers.at(index);
    }

    return nullptr;
}
