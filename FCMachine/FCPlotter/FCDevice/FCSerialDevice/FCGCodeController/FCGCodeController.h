#ifndef FC_GCODE_CONTROLLER_H
#define FC_GCODE_CONTROLLER_H

#include <QObject>
#include <QList>
#include <QColor>
#include <QByteArray>
#include <QSerialPort>
#include "FCSerialDevice.h"
#include "FCM115.h"

/**
 * @brief Драйвер управления контроллером Marlin.
 * @details Парсит ответы, управляет RGB, предоставляет аварийные команды.
 *          ВАЖНО: Дублирующий _state удалён. Используется FCDevice::state().
 * @warning Блокирующие операции вызывать из рабочего потока.
 */
class FCGCodeController
    : public FCSerialDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCGCodeController)
public:
    QByteArray SecurityCode = {"fldskfks;lfk;sl"};

    explicit FCGCodeController(const QString &portName, QObject *parent = nullptr);
    ~FCGCodeController() override;

    [[nodiscard]] bool checkConnection();
    [[nodiscard]] const FCM115 &firmwareData() const noexcept { return _m115; }
    [[nodiscard]] bool setLed(const QList<QColor> &colors, uint8_t brightness = 255);

    // M112 - emergencyStop
    inline void stop() { send("M112"); }
    inline void disableMotors() { send("M84"); }
    inline void reboot() { send("M999"); }
    bool isSecretCheck();

signals:
//    void colorChanged(const QColor &color);

protected:
    bool init();
    bool final();
    [[nodiscard]] bool parse(const QString &response);
    [[nodiscard]] bool setParameters(QSerialPort *port);

private:
    void parseFirmwareData(const QString &response);
    FCM115 _m115;
    bool _connected = false;
};

using FCColorList = QList<QColor>;
using FCMarlinControllerList = QList<FCGCodeController *>;

#endif // FC_GCODE_CONTROLLER_H
