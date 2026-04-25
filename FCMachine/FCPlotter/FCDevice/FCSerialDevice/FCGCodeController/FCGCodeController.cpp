#include "FCGCodeController.h"
#include <QThread>
#include <QRegularExpression>

FCGCodeController::FCGCodeController(const QString &portName, QObject *parent)
: FCSerialDevice(portName, parent), _m115(), _connected(false)
{
    init();
}

FCGCodeController::~FCGCodeController()
{
    if(_connected)
    {
        final();
//        emergencyStop();
//        disconnect();
    }
}

bool FCGCodeController::init()
{
    if(!FCSerialDevice::init() && !checkConnection())
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Connection), objectName());
        return false;
    }
    else
    {
        _connected = true;
        emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    }
    return true;
}

bool FCGCodeController::final()
{
    if(_connected)
    {
        emergencyStop();
    }
    _connected = false;
    emit condition(state().set(FCReadyState::NotReady, FCErrorType::None), objectName());
    return FCSerialDevice::final();
}

bool FCGCodeController::checkConnection()
{
    bool success = send("M105", 1000, 1);
    if(success)
    {
        parseFirmwareData(answer());
    }
    return success;
}

bool FCGCodeController::setLed(const QList<QColor> &colors, uint8_t brightness)
{
    if(colors.isEmpty() || !state().is(FCReadyState::Ready))
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Connection), objectName());
        return false;
    }

    QStringList commands;
    commands.reserve(colors.size());
    for(int i = 0; i < colors.size(); ++i)
    {
        const QColor &c = colors.at(i);
        commands << QString("M150 R%1 U%2 B%3 P%4 I%5")
                        .arg(c.red()).arg(c.green()).arg(c.blue())
                        .arg(brightness).arg(i).toUtf8();
    }

    bool success = send(commands);
    if(success)
    {
        emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
        if(!colors.isEmpty())
        {
//            emit colorChanged(colors.first());
        }
    }
    else
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Write), objectName());
    }

    return success;
}

bool FCGCodeController::isSecretCheck()
{
    bool result = false;
    if(state().is(FCReadyState::Ready) && send("M916", 200, 1))
    {
        result = answer().trimmed().contains(SecurityCode) ? true : false;
    }

    return result;
}

bool FCGCodeController::parse(const QString &response)
{
    const QString trimmed = response.trimmed();
    if(trimmed.startsWith("Error:", Qt::CaseInsensitive) || trimmed.startsWith("ERROR:", Qt::CaseInsensitive))
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Parse), objectName());
        return false;
    }

    if(trimmed.startsWith("Resend:", Qt::CaseInsensitive))
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::None), objectName());
        return false;
    }

    if(trimmed.startsWith("ok", Qt::CaseInsensitive))
    {
        emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
        return true;
    }

    emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
    return true;
}

bool FCGCodeController::setParameters(QSerialPort *port)
{
    if(!port)
    {
        return false;
    }

    port->setBaudRate(QSerialPort::Baud115200);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
    port->setReadBufferSize(1024);

    return true;
}

void FCGCodeController::parseFirmwareData(const QString &response)
{
    if(!_m115.parse(response))
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Parse), objectName());
    }
}
