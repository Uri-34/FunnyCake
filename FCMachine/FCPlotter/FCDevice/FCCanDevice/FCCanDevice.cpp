#include "FCCanDevice.h"

FCCanDevice::FCCanDevice(const QString &name, QObject *parent)
    : FCDevice{name, parent}
{
    init();
}

bool FCCanDevice::init()
{
    _device = QCanBus::instance()->createDevice("socketcan", objectName());

//    if(_device->connectDevice())
//    {
//        connect(_device, &QCanBusDevice::framesReceived, this, [this]()
//        {
//            state().set(FCReadyState::NotReady);
//            while(_device->framesAvailable())
//            {
//                QCanBusFrame frame = _device->readFrame();
//            }
//            state().set(FCReadyState::Ready);
//        });

//        state().set(FCReadyState::Ready);
//        return true;
//    }
//    else
//    {
//        state().set(FCReadyState::NotReady);
//        return false;
//    }

    if(_device->connectDevice())
    {
        state().set(FCReadyState::Ready);
        return true;
    }
    else
    {
        state().set(FCReadyState::NotReady);
        return false;
    }
}

// 0x000–0x0FF : системные/высокоприоритетные
// 0x100–0x3FF : телеметрия
// 0x400–0x5FF : команды управления
// 0x600–0x7FF : диагностика/OBD-like
bool FCCanDevice::send(uint8_t command, const QByteArray &data)
{
    bool result = false;

    if(state().is(FCReadyState::Ready))
    {
        QCanBusFrame frame;
        frame.setFrameId(0x400); // 0x400–0x5FF : команды управления
        frame.setFrameType(QCanBusFrame::DataFrame);

        QByteArray _data;
        _data.append(command);
        _data.append(data);

        frame.setPayload(_data);

        result = _device->writeFrame(frame);
    }

    return result;
}

const QByteArray FCCanDevice::receive()
{
    return state().is(FCReadyState::Ready) ? _device->readFrame().payload() : QByteArray{};
}

QByteArray FCCanDevice::exchange(uint8_t command, const QByteArray &data)
{
    if(send(command, data))
    {
        return receive();
    }

    return QByteArray{};
}
