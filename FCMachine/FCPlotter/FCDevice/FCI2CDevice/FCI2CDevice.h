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
//    static const int FCI2CDeviceTimeOut = 100;

    explicit FCI2CDevice(FCI2CBus *bus, uint8_t address, const QString &name = QString(), QObject *parent = nullptr);
    ~FCI2CDevice() override = default;

    [[nodiscard]] inline QString path() const { return _bus ? _bus->path() : QString(); }
    [[nodiscard]] inline bool isOpen() const { return _bus ? _bus->isOpen() : false; }

    // отправка данных на устройство
    bool send(const QByteArray &data, FCI2CFlag flag = FCI2CFlag::None);
    bool send(uint8_t command, const QByteArray &data = QByteArray{}, FCI2CFlag flag = FCI2CFlag::None);
    // получение данных с устройства
    const QByteArray receive(int length, FCI2CFlag flag = FCI2CFlag::None);

    // обмен данными с устройством send(...) & receive(...)
    QByteArray exchange(uint8_t command, const QByteArray &data = QByteArray{}, int length = 0, FCI2CFlag flag = FCI2CFlag::None);

protected:
    [[nodiscard]] inline FCI2CBus* bus() { return _bus; }
    [[nodiscard]] inline uint8_t address() { return _address; }

    bool init();
    bool final();

private:
    QByteArray buildPacket(uint8_t command, const QByteArray &data);
    // Стандартный CRC8 (полином 0x07, инициализация 0x00)
    uint8_t crc8(const QByteArray &data);

    FCI2CBus *_bus = nullptr;
    uint8_t _address = 0;
};

using FCI2CDeviceList = QList<FCI2CDevice *>;
using FCI2CDeviceQList = QList<QPointer<FCI2CDevice>>;

static_assert(sizeof(uint8_t) == 1, "uint8_t должен быть ровно 1 байтом");

#endif // FC_I2C_DEVICE_H
