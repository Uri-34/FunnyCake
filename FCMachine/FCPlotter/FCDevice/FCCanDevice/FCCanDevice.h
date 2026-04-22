#ifndef FC_CAN_DEVICE_H
#define FC_CAN_DEVICE_H

#include <QCanBus>
#include "FCDevice.h"

/**
 * @brief класс для устройств работающих по can шине
 * @details Делегирует операции шине, валидирует адрес, управляет состоянием (2 компонента).
 * @warning Inline-методы проверяют _bus != nullptr. При ошибке устанавливают состояние.
 */
class FCCanDevice
    : public FCDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCCanDevice)
public:
//    static const int FCCanDeviceTimeOut = 100;
    explicit FCCanDevice(const QString &name = QString(), QObject *parent = nullptr);
    ~FCCanDevice() override = default;

    // отправка данных на устройство
    bool send(uint8_t command, const QByteArray &data = QByteArray{});
    // получение данных с устройства
    const QByteArray receive();
    // обмен данными с устройством send(...) & receive(...)
    QByteArray exchange(uint8_t command, const QByteArray &data = QByteArray{});

protected:
    bool init();

private:
    QCanBusDevice *_device = nullptr;
//    QByteArray _buffer;
};

using FCCanDeviceList = QList<FCCanDevice *>;
using FCCanDeviceQList = QList<QPointer<FCCanDevice>>;

#endif // FC_CAN_DEVICE_H
