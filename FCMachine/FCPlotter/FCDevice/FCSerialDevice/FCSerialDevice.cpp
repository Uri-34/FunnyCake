#include "FCSerialDevice.h"
#include <QThread>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QSerialPortInfo>

FCSerialDevice::FCSerialDevice(const QString &portName, QObject *parent)
: FCDevice(portName, parent)
{
    init(); // настраивает обьект
}

FCSerialDevice::~FCSerialDevice()
{
    final(); // гарантирует закрытие порта до удаления QObject
}

bool FCSerialDevice::init()
{
    QMutexLocker locker(&_portMutex);
    if(objectName().isEmpty() || !available())
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Configuration), objectName());
        return false;
    }

    _port = new QSerialPort(objectName(), this);
    if(!_port->open(QSerialPort::ReadWrite))
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Connection), objectName());

        delete _port;
        _port = nullptr;
        return false;
    }

    flush();
    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return true;
}

bool FCSerialDevice::final()
{
    QMutexLocker locker(&_portMutex);
    if(_port)
    {
        if(_port->isOpen())
        {
            _port->waitForBytesWritten(100);
            _port->close();
        }
        delete _port;
        _port = nullptr;
    }
    _buffer.clear();
    _cachedAnswer.clear();
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::None), objectName());
    disconnect();
    return true;
}

bool FCSerialDevice::send(const QString &command, int timeoutMs, int retries)
{
    for(int attempt = 0; attempt < retries; ++attempt)
    {
        if(writeBytesRaw(command, timeoutMs))
        {
            emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
            return true;
        }
        if(attempt < retries - 1)
        {
            QThread::msleep(50 * (attempt + 1));
        }
    }
    state().set(FCReadyState::NotReady, FCErrorType::Write);
    emit condition(state());
    return false;
}

bool FCSerialDevice::send(const QStringList &commands, int timeoutMs, int retries)
{
    for(auto  &command : commands)
    {
        if(!send(command, timeoutMs, retries))
        {
            emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
            return false;
        }
    }
    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return true;
}

bool FCSerialDevice::writeBytesRaw(const QString &bytes, int timeoutMs)
{
    QMutexLocker locker(&_portMutex);
    if(!state().is(FCReadyState::Ready))
    {
        return false;
    }

    _buffer.clear();
    const QByteArray data = bytes.toUtf8() + "\n";
    _port->write(data);
    _port->waitForBytesWritten(100);

    QElapsedTimer timer;
    timer.start();
    while(timer.elapsed() < timeoutMs)
    {
        if(_port->waitForReadyRead(50))
        {
            _buffer.append(_port->readAll());
            _cachedAnswer = QString::fromUtf8(_buffer);
            if(_buffer.contains("ok") || _buffer.contains("error") || _buffer.contains("ERROR"))
            {
                emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
                return true;
            }
        }
    }
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::Read), objectName());
    return false;
}

void FCSerialDevice::flush()
{
    QMutexLocker locker(&_portMutex);
    if(_port && _port->isOpen())
    {
        _port->clear(QSerialPort::AllDirections);
    }
    _buffer.clear();
}

bool FCSerialDevice::available() const
{
    const auto ports = QSerialPortInfo::availablePorts();
    for(const QSerialPortInfo &info : ports)
    {
        if(info.systemLocation() == objectName() || info.portName() == objectName())
        {
            return true;
        }
    }
    return false;
}
