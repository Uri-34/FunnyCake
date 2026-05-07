#include "FCGCodeController.h"
#include <QThread>
#include <QRegularExpression>

FCGCodeController::FCGCodeController(const QString &portName, QObject *parent)
    : FCSerialDevice(portName, parent)
{}

FCGCodeController::~FCGCodeController()
{}

void FCGCodeController::onStart(const QString &model)
{
    Q_UNUSED(model);

    // останавливаем перемещение M112 - emergencyStop --- ??????????????
    send("M112");
    // аварийно чистим буффер комманд плоттера
    clear();
    // поднимаем Z до транспорной высоты и "едем в дом", там опускаем головку по Z в 0
    send("G0 Z20");
    send("G28 X Y");
    send("G0 Z0");

    condition(state().set(FCPlayState::Start), this);
    emit started("запущен", this);
}

void FCGCodeController::onStop()
{
    // останавливаем перемещение M112 - emergencyStop --- ??????????????
    send("M112");
    // аварийно чистим буффер комманд плоттера
    clear();

    condition(state().set(FCPlayState::Stop), this);
    emit stoped("остановлен", this);
}

void FCGCodeController::onPause()
{
    // делаем паузу по возможности быстро остановив перемещения, остановив насос, остановив головку
    // нужно разобраться - возможно ли это ???
    // если это проблемно - может быть вообще выкинуть режим паузы ???

    condition(state().set(FCPlayState::Pause), this);
    emit stoped("приостановлен", this);
}

//void FCGCodeController::onReset()
//{
//    // общий сброс
//    send("M999");

//    clear();

//    condition(state().set(FCPlayState::Stop), this);
//    emit stoped("сброшен", this);
//}

void FCGCodeController::onTest()
{
    emit message("запущен тест", this);

    // реализовать ...

    emit message("тест завершен", this);
}

bool FCGCodeController::checkConnection()
{
    if(state().is(FCOpenState::Open, FCReadyState::Ready))
    {
        return send("M105", 1000, 1);
    }

    return false;
}

void FCGCodeController::setLed(const QList<QColor> &colors, uint8_t brightness)
{
    if(!colors.isEmpty() || state().is(FCOpenState::Open, FCReadyState::Ready))
    {
        QStringList commands;
        commands.reserve(colors.size());
        for(int i = 0; i < colors.size(); ++i)
        {
            const QColor &c = colors.at(i);
            commands << QString("M150 R%1 U%2 B%3 P%4 I%5")
                            .arg(c.red()).arg(c.green()).arg(c.blue())
                            .arg(brightness).arg(i).toUtf8();
        }

        send(commands);
    }
}

bool FCGCodeController::isSecretCheck()
{
    if(state().is(FCOpenState::Open, FCReadyState::Ready) && send("M916", 200, 1))
    {
        return receive().trimmed().contains(SecurityCode) && _m115.isCompatible() ? true : false;
    }

    return false;
}

void FCGCodeController::parse(const QString &string)
{
    const QString trimmed = string.trimmed();
    if(trimmed.startsWith("Error:", Qt::CaseInsensitive) || trimmed.startsWith("ERROR:", Qt::CaseInsensitive))
    {
        state().set(FCReadyState::NotReady, FCErrorType::Parse);
    }

    if(trimmed.startsWith("Resend:", Qt::CaseInsensitive))
    {
        state().set(FCReadyState::NotReady, FCErrorType::None);
    }

    if(trimmed.startsWith("ok", Qt::CaseInsensitive))
    {
        state().set(FCReadyState::Ready, FCErrorType::None);
    }

    condition(state(), this);
}

void FCGCodeController::parseFirmwareData(const QString &response)
{
    if(!_m115.parse(response))
    {
        condition(state().set(FCReadyState::NotReady, FCErrorType::Parse), this);
    }
}
