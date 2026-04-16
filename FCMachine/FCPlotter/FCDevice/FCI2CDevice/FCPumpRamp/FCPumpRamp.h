#ifndef FC_PUMP_RAMP_H
#define FC_PUMP_RAMP_H

#include <QObject>
#include <QList>
#include <QColor>
#include "FCI2CDevice.h"
#include "FCLM75AThermometer.h"

/**
 * @brief Контроллер насосной рампы с датчиками температуры.
 * @details Управляет 3 насосами по цвету, мониторит температуру через LM75A.
 *          Гарантирует отключение всех насосов перед включением нового.
 */
class FCPumpRamp
    : public FCI2CDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCPumpRamp)
public:
    explicit FCPumpRamp(FCI2CBus *bus, QObject *parent = nullptr);
    ~FCPumpRamp();

    // читаем температуру с термометра number
    [[nodiscard]] qreal thermometer(int number) const;
    [[nodiscard]] QString securityCode(int timeoutMs) { Q_UNUSED(timeoutMs); return {}; }

public slots:
    bool switchTo(uint8_t pumpNumber);
    bool reset();

signals:
    void pumpSwitched(uint8_t pumpNumber);
    void temperatureChanged(uint8_t index, qreal temperature);

protected:
    bool init();
    bool final();

private:
//    [[nodiscard]] uint8_t selectPumpNumber(const QColor &color) const;
    const QList<uint8_t> _pumps;
    QList<FCLM75AThermometer*> _thermometers;
};

#endif // FC_PUMP_RAMP_H
