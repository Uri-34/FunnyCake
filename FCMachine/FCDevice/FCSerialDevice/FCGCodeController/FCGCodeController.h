#ifndef FC_GCODE_CONTROLLER_H
#define FC_GCODE_CONTROLLER_H

#include <QObject>
#include <QList>
#include <QColor>
#include <QByteArray>
#include <QSerialPort>
#include "FCSerialDevice.h"
#include "FCM115.h"

class FCGCodeController
    : public FCSerialDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCGCodeController)
public:
    const QByteArray SecurityCode = {"fldskfks;lfk;sl"};

    explicit FCGCodeController(const QString &portName, QObject *parent = nullptr);
    ~FCGCodeController() override;

    bool checkConnection();
    const FCM115 &firmwareData() const noexcept { return _m115; }
    void setLed(const QList<QColor> &colors, uint8_t brightness = 255);

    bool isSecretCheck();

    inline bool readiness() override { return state().is(FCOpenState::Open, FCReadyState::Ready) && checkConnection() && isSecretCheck(); }

public slots:
    void onStart(const QString &model) override;
    void onStop() override;
    void onPause() override;

//    void onReset() override;

    void onTest() override;

protected:
    void parse(const QString &response);

private:
    void parseFirmwareData(const QString &response);
    FCM115 _m115;
};

using FCColorList = QList<QColor>;
using FCMarlinControllerList = QList<FCGCodeController *>;

#endif // FC_GCODE_CONTROLLER_H
