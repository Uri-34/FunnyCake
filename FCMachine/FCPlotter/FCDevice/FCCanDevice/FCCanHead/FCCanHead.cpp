#include "FCCanHead.h"

bool FCCanHead::init()
{
    if(!device()->connectDevice() && securityCode() != SecurityCode)
    {
        state().set(FCReadyState::NotReady);
        return false;
    }

    _feedersCount = feedersCount();

    connect(device(), &QCanBusDevice::stateChanged, this, [this](QCanBusDevice::CanBusDeviceState state)
                                                          {
                                                                switch(state)
                                                                {
                                                                    case QCanBusDevice::CanBusDeviceState::ConnectedState:
                                                                        this->state().set(FCReadyState::Ready);
                                                                    break;
                                                                    default:
                                                                        this->state().set(FCReadyState::NotReady);
                                                                 }
                                                          });

    connect(device(), &QCanBusDevice::errorOccurred, this, [this](QCanBusDevice::CanBusError error)
                                                           {
                                                                this->state().set(FCReadyState::NotReady);
                                                                switch(error)
                                                                {
                                                                    case QCanBusDevice::CanBusError::ConfigurationError:
                                                                        this->state().set(FCErrorType::Configuration);
                                                                    break;
                                                                    case QCanBusDevice::CanBusError::ConnectionError:
                                                                        this->state().set(FCErrorType::Connection);
                                                                    break;
                                                                    case QCanBusDevice::CanBusError::OperationError:
                                                                        this->state().set(FCErrorType::Motion);
                                                                    break;
                                                                    case QCanBusDevice::CanBusError::NoError:
                                                                        this->state().set(FCErrorType::None);
                                                                    break;
                                                                    case QCanBusDevice::CanBusError::ReadError:
                                                                        this->state().set(FCErrorType::Read);
                                                                    break;
                                                                    case QCanBusDevice::CanBusError::TimeoutError:
                                                                        this->state().set(FCErrorType::Timeout);
                                                                    break;
                                                                    case QCanBusDevice::CanBusError::UnknownError:
                                                                        this->state().set(FCErrorType::Motion);
                                                                    break;
                                                                    case QCanBusDevice::CanBusError::WriteError:
                                                                        this->state().set(FCErrorType::Write);
                                                                    break;
                                                                }
                                                           });

    // ...

    state().set(FCReadyState::Ready);
    return true;
}
