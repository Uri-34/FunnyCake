#include "FCI2CHead.h"

bool FCI2CHead::init()
{
    if(!controllerState().isReady() || securityCode() != SecurityCode)
    {
        state().set(FCReadyState::NotReady);
        return false;
    }

    _feedersCount = feedersCount();

    // ...

    state().set(FCReadyState::Ready);
    return true;
}

void FCI2CHead::switchToFeeder(uint8_t number)
{
    QByteArray n;
    n.append(static_cast<uint8_t>(FCRange<uint8_t>(0, _feedersCount).clamped(number)));
    send(SwitchToFeeder, n);
}
