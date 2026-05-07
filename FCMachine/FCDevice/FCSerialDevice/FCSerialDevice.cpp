#include "FCSerialDevice.h"
#include <QThread>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QSerialPortInfo>

FCSerialDevice::FCSerialDevice(const QString &portName, QObject *parent)
    : FCDevice(portName, parent)
{
    init();
}

FCSerialDevice::~FCSerialDevice()
{
    final();
}

bool FCSerialDevice::init()
{
    QMutexLocker locker(&_portMutex);

    bool result = false;
    _port = new QSerialPort(objectName(), this);
    if(!_port->open(QSerialPort::ReadWrite))
    {
        delete _port;
        _port = nullptr;

        state().set(FCOpenState::Close, FCReadyState::NotReady, FCErrorType::Open);

        result = false;
    }

    if(_port && _port->isOpen())
    {
        _port->setBaudRate(QSerialPort::Baud115200);
        _port->setDataBits(QSerialPort::Data8);
        _port->setParity(QSerialPort::NoParity);
        _port->setStopBits(QSerialPort::OneStop);
        _port->setFlowControl(QSerialPort::NoFlowControl);
        _port->setReadBufferSize(1024);

        state().set(FCOpenState::Open, FCReadyState::Ready);

        result = true;
    }

    return result;
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
    clear();
    disconnect();

    return true;
}

bool FCSerialDevice::send(const QString &command, int timeoutMs, int retries)
{
    for(int count = 0; count < retries; count++)
    {
        if(writeBytesRaw(command, timeoutMs))
        {
            return true;
        }

        QThread::msleep(50 * (count + 1));
    }

    return false;;
}

bool FCSerialDevice::send(const QStringList &commands, int timeoutMs, int retries)
{
    for(auto  &command : commands)
    {
        if(!send(command, timeoutMs, retries))
        {
            return false;
        }
    }
    return true;
}

bool FCSerialDevice::writeBytesRaw(const QString &bytes, int timeoutMs)
{
    QMutexLocker locker(&_portMutex);
    if(!state().is(FCOpenState::Open, FCReadyState::Ready, FCPlayState::Start))
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
            _answer = QString::fromUtf8(_buffer);
            if(_buffer.contains("ok") || _buffer.contains("error") || _buffer.contains("ERROR"))
            {
                emit condition(state().set(FCReadyState::Ready, FCErrorType::None), this);
                return true;
            }
        }
    }
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::Read), this);
    return false;
}

void FCSerialDevice::clear()
{
    QMutexLocker locker(&_portMutex);
    if(_port && _port->isOpen())
    {
        _port->clear(QSerialPort::AllDirections);
    }
    _buffer.clear();
    _answer.clear();
}

bool FCSerialDevice::portAvailable()
{
    for(QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        if(info.systemLocation() == objectName() || info.portName() == objectName())
        {
            return true;
        }
    }
    return false;
}

