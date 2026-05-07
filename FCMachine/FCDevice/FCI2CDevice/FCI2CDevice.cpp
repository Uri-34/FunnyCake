#include "FCI2CDevice.h"

FCI2CDevice::FCI2CDevice(FCI2CBus *bus, uint8_t address, const QString &name, QObject *parent)
    : FCDevice(name, parent),
      _bus(bus),
      _address(address)
{
    init();
}

void FCI2CDevice::init()
{
    if(!bus() || !bus()->isOpen() || !FCRange<uint8_t>(0x00, 0x7F).contains(_address))
    {
        state().set(FCOpenState::Close, FCReadyState::NotReady, FCErrorType::Connection);
    }
    else
    {
        state().set(FCOpenState::Open, FCReadyState::Ready, FCErrorType::None);
    }

    condition(state(), this);
}

void FCI2CDevice::final()
{
    if(_bus->isOpen())
    {
        _bus->close();
    }

    condition(state().set(FCOpenState::Close, FCReadyState::NotReady), this);
}

QByteArray FCI2CDevice::buildPacket(uint8_t command, const QByteArray &data)
{
    QByteArray packet;
    packet.append(command);
    packet.append(static_cast<char>(data.size()));
    packet.append(address());
    packet.append(data);
    packet.append(static_cast<char>(crc8(packet)));

    return packet;
}

uint8_t FCI2CDevice::crc8(const QByteArray &data)
{
    uint8_t crc = 0x00;
    for(char b : data)
    {
        crc ^= static_cast<uint8_t>(b);
        for (int count = 0; count < 8; ++count)
        {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
        }
    }
    return crc;
}

bool FCI2CDevice::send(uint8_t command, const QByteArray &data, FCI2CFlag flag)
{
    if(!state().is(FCOpenState::Open, FCReadyState::Ready))
    {
        return false;
    }

    bool result = bus()->writeBytes(address(), buildPacket(command, data), flag);
    if(!result)
    {
        state().set(FCReadyState::NotReady, FCErrorType::Write);
    }
    else
    {
        state().set(FCReadyState::Ready, FCErrorType::None);
    }

    condition(state(), this);
    return result;
}

bool FCI2CDevice::send(const QByteArray &data, FCI2CFlag flag)
{
    if(!state().is(FCOpenState::Open, FCReadyState::Ready))
    {
        return false;
    }

    bool result = bus()->writeBytes(address(), data, flag);
    if(!result)
    {
        state().set(FCReadyState::NotReady, FCErrorType::Write);
    }
    else
    {
        state().set(FCReadyState::Ready, FCErrorType::None);
    }

    condition(state(), this);
    return result;
}

const QByteArray FCI2CDevice::receive(int length, FCI2CFlag flag)
{
    if(!state().is(FCReadyState::Ready))
    {
        condition(state(), this);
        return QByteArray{};
    }

    QByteArray buffer = bus()->readBytes(address(), length, flag);
    if(buffer.isEmpty())
    {
        state().set(FCReadyState::NotReady, FCErrorType::Write);
    }
    else
    {
        state().set(FCReadyState::Ready, FCErrorType::None);
    }

    condition(state(), this);
    return buffer;
}

QByteArray FCI2CDevice::exchange(uint8_t command, const QByteArray &data, int length, FCI2CFlag flag)
{
    if(!state().is(FCOpenState::Open, FCReadyState::Ready))
    {
        condition(state(), this);
        return QByteArray{};
    }

    QByteArray buffer = bus()->writeRead(address(), buildPacket(command, data), length, flag);
    if(!buffer.isEmpty())
    {
        state().set(FCReadyState::NotReady, FCErrorType::Write);
    }
    else
    {
        state().set(FCReadyState::Ready, FCErrorType::None);
    }

    condition(state(), this);
    return buffer;
}
