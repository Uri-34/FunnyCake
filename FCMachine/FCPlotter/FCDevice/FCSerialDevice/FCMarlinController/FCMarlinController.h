#ifndef FC_MARLIN_CONTROLLER_H
#define FC_MARLIN_CONTROLLER_H

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
class FCMarlinController
    : public FCSerialDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCMarlinController)
public:
    explicit FCMarlinController(const QString &portName, QObject *parent = nullptr);
    ~FCMarlinController() override;

    [[nodiscard]] bool checkConnection();
    [[nodiscard]] const FCM115 &firmwareData() const noexcept { return _m115; }
    [[nodiscard]] bool setLed(const QList<QColor> &colors, uint8_t brightness = 255);

    inline void emergencyStop() { sendCommand("M112"); }
    inline void disableMotors() { sendCommand("M84"); }
    inline void reboot() { sendCommand("M999"); }
    [[nodiscard]] QString securityCode(int timeoutMs) override;

signals:
    void colorChanged(const QColor &color);

protected:
    bool init();
    bool final();
    [[nodiscard]] bool parse(const QString &response);
    [[nodiscard]] bool setupPortParameters(QSerialPort *port);

private:
    void parseFirmwareData(const QString &response);
    FCM115 _m115;
    bool _connected = false;
};

using FCColorList = QList<QColor>;
using FCMarlinControllerList = QList<FCMarlinController *>;

#endif // FC_MARLIN_CONTROLLER_H
