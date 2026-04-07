#ifndef FC_I2C_DEVICE_H
#define FC_I2C_DEVICE_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <cstdint>

#include "FCDevice.h"
#include "FCI2CBus.h"
#include "FCRange.h"

/**
 * @brief Базовый класс для устройств I²C.
 * @details Делегирует операции шине, валидирует адрес, управляет состоянием (2 компонента).
 * @warning Inline-методы проверяют _bus != nullptr. При ошибке устанавливают состояние.
 */
class FCI2CDevice
    : public FCDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCI2CDevice)
public:
    explicit FCI2CDevice(FCI2CBus *bus, uint8_t address, const QString &name = QString(), QObject *parent = nullptr);
    ~FCI2CDevice() override = default;

    [[nodiscard]] inline QString path() const { return _bus ? _bus->path() : QString(); }
    [[nodiscard]] inline bool isOpen() const { return _bus ? _bus->isOpen() : false; }

    // Делегирующие методы с защитой от nullptr
    uint8_t readByte();
    bool writeByte(uint8_t byte);
    QByteArray readBytes(int count, int flags = 0);
    bool writeBytes(const QByteArray &data, int flags = 0);
    uint8_t readRegister(uint8_t reg);
    bool writeRegister(uint8_t reg, uint8_t value);
    QByteArray writeRead(const QByteArray &data, int count, int flags = 0);

protected:
    [[nodiscard]] inline FCI2CBus* bus() { return _bus; }

    bool init();
    bool final();

private:
    FCI2CBus *_bus = nullptr;
    uint8_t _address = 0;
};

using FCI2CDeviceList = QList<FCI2CDevice *>;
using FCI2CDeviceQList = QList<QPointer<FCI2CDevice>>;

static_assert(sizeof(uint8_t) == 1, "uint8_t должен быть ровно 1 байтом");

#endif // FC_I2C_DEVICE_H
