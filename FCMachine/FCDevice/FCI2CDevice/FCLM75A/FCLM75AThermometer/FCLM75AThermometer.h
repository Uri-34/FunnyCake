#ifndef FC_LM75A_THERMOMETER_H
#define FC_LM75A_THERMOMETER_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <cmath>
#include <limits>
#include "FCI2CDevice.h"

/**
 * @brief Драйвер датчика температуры LM75A.
 * @details Поддерживает режимы отключения, калибровку и пороговую эмиссию.
 *          Состояние управляется строго через FCDevice::set() (2 компонента).
 * @warning Блокирующие вызовы использовать в рабочем потоке.
 */
class FCLM75AThermometer
    : public FCI2CDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCLM75AThermometer)
public:
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x48;
    static constexpr int MIN_CONVERSION_TIME_MS = 27;
    static constexpr int DEFAULT_RESOLUTION_BITS = 9;
    static constexpr float DEFAULT_COMPENSATION = 1.0f;
    static constexpr float DEFAULT_DELTA_THRESHOLD = 0.5f;
    static constexpr uint8_t REG_TEMPERATURE = 0x00;
    static constexpr uint8_t REG_CONFIGURATION = 0x01;

    explicit FCLM75AThermometer(FCI2CBus *bus, uint8_t address = DEFAULT_I2C_ADDRESS,
                                const QString &name = QString(), QObject *parent = nullptr);
    ~FCLM75AThermometer() override;

    [[nodiscard]] float temperatureC();
//    bool setShutdownMode(bool enable);
//    [[nodiscard]] bool isDataReady();

    void setCompensation(float offset) noexcept { _compensation = offset; }
    [[nodiscard]] float compensation() const noexcept { return _compensation; }
    void setDeltaThreshold(float delta) noexcept { _deltaThreshold = std::abs(delta); }
    [[nodiscard]] float deltaThreshold() const noexcept { return _deltaThreshold; }

//    [[nodiscard]] int16_t readRegister(uint8_t reg);
//    [[nodiscard]] bool writeRegister(uint8_t reg, uint8_t value);


    [[nodiscard]] QString securityCode(int timeoutMs) { Q_UNUSED(timeoutMs); return {}; }

signals:
    void temperatureChanged(float celsius);

protected:
    bool init();
    bool final();

private:
    [[nodiscard]] bool configure(int resolutionBits, bool shutdown);
    float _compensation = DEFAULT_COMPENSATION;
    float _deltaThreshold = DEFAULT_DELTA_THRESHOLD;
    float _lastTemperature = std::numeric_limits<float>::quiet_NaN();
    mutable QElapsedTimer _lastReadTimer;
};

using FCLM75AThermometerList = QList<FCLM75AThermometer*>;
using FCLM75AThermometerQList = QList<QPointer<FCLM75AThermometer>>;
static_assert(FCLM75AThermometer::DEFAULT_I2C_ADDRESS <= 0x7F, "LM75A адрес должен быть 7-битным");
#endif // FC_LM75A_THERMOMETER_H
