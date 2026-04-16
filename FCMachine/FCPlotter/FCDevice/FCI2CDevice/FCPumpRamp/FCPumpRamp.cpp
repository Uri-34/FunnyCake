#include "FCPumpRamp.h"

FCPumpRamp::FCPumpRamp(FCI2CBus *bus, QObject *parent)
: FCI2CDevice(bus, 0x20, "Ramp", parent),
  _pumps{0xFE, 0xFD, 0xFB}
{
    init();
}

FCPumpRamp::~FCPumpRamp()
{
    final();
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
    reset();
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::None), objectName());
    return true;
}

bool FCPumpRamp::reset()
{
    QByteArray off{static_cast<char>(0xFF), static_cast<char>(0xFF)};
    if(send(off))
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

bool FCPumpRamp::switchTo(uint8_t pumpNumber)
{
    if(!reset())
    {
        return false;
    }

    QByteArray number{static_cast<char>(_pumps.at(pumpNumber)), static_cast<char>(0xFF)};
    if(send(number))
    {
        emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
        emit pumpSwitched(pumpNumber);

        return true;
    }
    else
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());

        return false;
    }
}

qreal FCPumpRamp::thermometer(int index) const
{
    if(FCRange<int>(0, _thermometers.size()).contains(index))
    {
        return _thermometers.at(index)->temperatureC();
    }

    return 0.0;
}
