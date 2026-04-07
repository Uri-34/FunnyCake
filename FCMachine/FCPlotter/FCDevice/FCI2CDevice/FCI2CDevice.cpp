#include "FCI2CDevice.h"

FCI2CDevice::FCI2CDevice(FCI2CBus *bus, uint8_t address, const QString &name, QObject *parent)
    : FCDevice(name, parent),
      _bus(bus),
      _address(address)
{
    init();
}

bool FCI2CDevice::init()
{
    if (!bus() || !bus()->isOpen() || !FCRange<uint8_t>(0x00, 0x7F).contains(_address))
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Connection), objectName());
        return false;
    }

    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return true;
}

bool FCI2CDevice::final()
{
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::None), objectName());
    return true;
}

uint8_t FCI2CDevice::readByte()
{
    uint8_t retCode = bus()->readByte(_address);

    if(retCode)
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Read), objectName());
    }

    return retCode;
}

bool FCI2CDevice::writeByte(uint8_t byte)
{
    uint8_t retCode = bus()->writeByte(_address, byte);

    if(retCode)
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
    }

    return retCode;
}

QByteArray FCI2CDevice::readBytes(int count, int flags)
{
    QByteArray retArray = bus()->readBytes(_address, count, flags);

    if(retArray.isEmpty())
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Read), objectName());
    }

    return retArray;
}

bool FCI2CDevice::writeBytes(const QByteArray &data, int flags)
{
    bool retCode = bus()->writeBytes(_address, data, flags);

    if(!retCode)
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
    }

    return retCode;
}

uint8_t FCI2CDevice::readRegister(uint8_t reg)
{
    uint8_t retCode = bus()->readRegister(_address, reg);

    if(retCode)
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Read), objectName());
    }

    return retCode;
}

bool FCI2CDevice::writeRegister(uint8_t reg, uint8_t value)
{
    bool retCode = bus()->writeRegister(_address, reg, value);

    if(!retCode)
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
    }

    return retCode;
}

QByteArray FCI2CDevice::writeRead(const QByteArray &data, int count, int flags)
{
    QByteArray retArray = bus()->writeRead(_address, data, count, flags);

    if(retArray.isEmpty())
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
    }

    return retArray;
}

