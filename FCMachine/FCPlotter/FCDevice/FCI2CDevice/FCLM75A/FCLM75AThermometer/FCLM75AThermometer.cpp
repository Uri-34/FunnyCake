#include "FCLM75AThermometer.h"
#include <QThread>

FCLM75AThermometer::FCLM75AThermometer(FCI2CBus *bus, uint8_t address, const QString &name, QObject *parent)
: FCI2CDevice(bus, address, name, parent)
{
    _lastReadTimer.start();
    init();
}

FCLM75AThermometer::~FCLM75AThermometer()
{
    setShutdownMode(true);
}

bool FCLM75AThermometer::init()
{
    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return configure(DEFAULT_RESOLUTION_BITS, true);
}

bool FCLM75AThermometer::final()
{
    setShutdownMode(true);
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::None), objectName());
    return true;
}

bool FCLM75AThermometer::configure(int resolutionBits, bool shutdown)
{
    if(FCRange<int>(9, 12).excludes(resolutionBits))
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::OutOfRange), objectName());
        return false;
    }

    int16_t configValue = readRegister(REG_CONFIGURATION);
    if (configValue < 0)
    {
        return false;
    }

    uint8_t newConfig = static_cast<uint8_t>(configValue) & 0xF8;
    if(shutdown)
    {
        newConfig |= 0x01;
    }

    newConfig |= ((static_cast<uint8_t>(resolutionBits) - 9) & 0x03) << 1;

    if(!writeRegister(REG_CONFIGURATION, newConfig))
    {
        return false;
    }

    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return true;
}

float FCLM75AThermometer::temperatureC()
{
    qint64 elapsed = _lastReadTimer.elapsed();
    if(elapsed < MIN_CONVERSION_TIME_MS)
    {
        QThread::msleep(static_cast<unsigned long>(MIN_CONVERSION_TIME_MS - elapsed));
    }

    int16_t configReg = readRegister(REG_CONFIGURATION);
    if(configReg < 0)
    {
        return std::isnan(_lastTemperature) ? 0.0f : _lastTemperature;
    }

    bool isShutdown = (configReg & 0x01) != 0;
    if(isShutdown)
    {
        uint8_t newConfig = static_cast<uint8_t>(configReg) & ~0x01;
        if(!writeRegister(REG_CONFIGURATION, newConfig))
        {
            return std::isnan(_lastTemperature) ? 0.0f : _lastTemperature;
        }
        QThread::msleep(MIN_CONVERSION_TIME_MS);
    }

    int16_t rawTemp = readRegister(REG_TEMPERATURE);
    if(rawTemp < 0)
    {
        return std::isnan(_lastTemperature) ? 0.0f : _lastTemperature;
    }

    float temperature = static_cast<float>(static_cast<int16_t>(rawTemp)) / 128.0f + _compensation;
    bool shouldEmit = std::isnan(_lastTemperature) || std::abs(temperature - _lastTemperature) >= _deltaThreshold;

    if(shouldEmit)
    {
        _lastTemperature = temperature;
        emit temperatureChanged(temperature);
    }
    _lastReadTimer.restart();
    return temperature;
}

bool FCLM75AThermometer::setShutdownMode(bool enable)
{
    int16_t configReg = readRegister(REG_CONFIGURATION);
    if (configReg < 0) return false;

    uint8_t newConfig = static_cast<uint8_t>(configReg);
    newConfig = enable ? (newConfig | 0x01) : (newConfig & ~0x01);
    if(!writeRegister(REG_CONFIGURATION, newConfig))
    {
        return false;
    }

    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return true;
}

bool FCLM75AThermometer::isDataReady()
{
    int16_t configReg = readRegister(REG_CONFIGURATION);
    if(configReg < 0)
    {
        return false;
    }

    return (configReg & 0x01) == 0;
}

int16_t FCLM75AThermometer::readRegister(uint8_t reg)
{
    QByteArray cmd(1, static_cast<char>(reg));
    if(!writeBytes(cmd))
    {
        return -1;
    }

    QByteArray data = readBytes(2);
    // ИСПРАВЛЕНО: была логическая ошибка || -> &&
    if(data.size() < 2)
    {
        return -1;
    }

    return (static_cast<uint8_t>(data[0]) << 8) | static_cast<uint8_t>(data[1]);
}

bool FCLM75AThermometer::writeRegister(uint8_t reg, uint8_t value)
{
    QByteArray cmd(2, '\0');
    cmd[0] = static_cast<char>(reg);
    cmd[1] = static_cast<char>(value);
    return writeBytes(cmd);
}
