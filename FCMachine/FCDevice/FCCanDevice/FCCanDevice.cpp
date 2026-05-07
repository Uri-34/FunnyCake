#include "FCCanDevice.h"

FCCanDevice::FCCanDevice(const QString &name, QObject *parent)
    : FCDevice{name, parent}
{
    init();
}

void FCCanDevice::init()
{
    if((_device = QCanBus::instance()->createDevice("socketcan", objectName()))->connectDevice())
    {
        state().set(FCOpenState::Open, FCReadyState::Ready);
    }
    else
    {
        state().set(FCOpenState::Close, FCReadyState::NotReady, FCErrorType::Connection);
    }
}

// 0x000–0x0FF : системные/высокоприоритетные
// 0x100–0x3FF : телеметрия
// 0x400–0x5FF : команды управления
// 0x600–0x7FF : диагностика/OBD-like
bool FCCanDevice::send(uint8_t command, const QByteArray &data)
{
    bool result = false;
    if(state().is(FCOpenState::Open, FCReadyState::Ready))
    {
        QCanBusFrame frame;
        frame.setFrameId(0x400); // 0x400–0x5FF : команды управления
        frame.setFrameType(QCanBusFrame::DataFrame);

        QByteArray _data;
        _data.append(command);
        _data.append(data);

        frame.setPayload(_data);

        if(_device->writeFrame(frame))
        {
            state().set(FCReadyState::Ready, FCErrorType::None);
            result = true;
        }
        else
        {
            state().set(FCReadyState::NotReady, FCErrorType::Write);
            result = false;
        }
    }

    emit condition(state(), this);

    return result;
}

const QByteArray FCCanDevice::receive()
{
    QByteArray answer;
    if(state().is(FCOpenState::Open, FCReadyState::Ready, FCErrorType::None))
    {
        answer = _device->readFrame().payload();
    }
    else
    {
        answer = QByteArray{};
    }

    emit condition(state(), this);

    return answer;
}

QByteArray FCCanDevice::exchange(uint8_t command, const QByteArray &data)
{
    if(send(command, data))
    {
        return receive();
    }

    return QByteArray{};
}
