#ifndef FC_PLOTTER_HARDWARE_H
#define FC_PLOTTER_HARDWARE_H

#include <QObject>
#include <QString>

#include "FCMarlinController.h"
#include "FCPumpRamp.h"
#include "FCI2CBus.h"
#include "FCCanHead.h"
#include "FCState.h"


class FCPlotterHardware
    : public QObject
{
    Q_OBJECT

public:
    explicit FCPlotterHardware(const QString &portName, FCI2CBus *bus, QObject *parent = nullptr);
    ~FCPlotterHardware() override;

    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] FCMarlinController* controller() const noexcept { return _controller; }
    [[nodiscard]] FCPumpRamp* ramp() const noexcept { return _ramp; }
    [[nodiscard]] FCI2CBus* bus() const noexcept { return _bus; }
    void emergencyStop();
    [[nodiscard]] QString securityCode(int timeoutMs) const;

    const FCCanHead& head() { return _head; }

signals:
    void condition(const FCPlotterHardwareState &state);

private:
    bool init();
    bool final();

    inline const FCPlotterHardwareState& state() { return _state; }

    QString _portName;
    FCI2CBus *_bus = nullptr;
    FCMarlinController *_controller = nullptr;
    FCPumpRamp *_ramp = nullptr;
    FCCanHead _head;

    FCPlotterHardwareState _state;
};

#endif // FC_PLOTTER_HARDWARE_H
