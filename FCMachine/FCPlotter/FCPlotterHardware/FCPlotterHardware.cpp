#include "FCPlotterHardware.h"

#include <QDebug>

FCPlotterHardware::FCPlotterHardware(const QString &portName, FCI2CBus *bus, QObject *parent)
    : QObject(parent),
      _portName(portName),
      _bus(bus),
      _controller(new FCMarlinController(portName, this)),
      _ramp(new FCPumpRamp(bus, this)),
      _head{FCHead{bus, 100, this}}
{
    init();
}

FCPlotterHardware::~FCPlotterHardware()
{
    final();
}

bool FCPlotterHardware::init()
{
//    connect(_bus, &FCI2CBus::condition, this,
//            [this](const FCBusState &state)
//            {
//                _state &= state;
//                emit condition(_state);
//            });
    connect(_controller, &FCMarlinController::condition, this,
            [this](const FCDeviceState &state)
            {
                _state &= state;
                emit condition(_state);
            });
    connect(_ramp, &FCPumpRamp::condition, this,
            [this](const FCDeviceState &state)
            {
                _state &= state;
                emit condition(_state);
            });

    // colorChanged() не смена цвета жижи а смена цвета индикатора
//    connect(_controller, &FCMarlinController::colorChanged, _ramp, &FCPumpRamp::switchTo);

    return true;
}

bool FCPlotterHardware::final()
{
    if (_controller)
    {
        _controller->emergencyStop();
        _controller->disableMotors();
        _controller->disconnect();
    }

    if (_ramp)
    {
        _ramp->reset();
    }

    _state.set(FCReadyState::NotReady, FCPlayState::Stop);
    emit condition(_state);

    return true;
}

bool FCPlotterHardware::isReady() const noexcept
{
    return _state.is(FCReadyState::Ready);
}

void FCPlotterHardware::emergencyStop()
{
    if(_controller)
    {
        _controller->emergencyStop();
    }

    if(_ramp)
    {
        _ramp->reset();
    }

    _state.set(FCReadyState::NotReady, FCErrorType::EmergencyStoped);
}

QString FCPlotterHardware::securityCode(int timeoutMs) const
{
    Q_UNUSED(timeoutMs);
    return QStringLiteral("SEC-") + _portName;
}
