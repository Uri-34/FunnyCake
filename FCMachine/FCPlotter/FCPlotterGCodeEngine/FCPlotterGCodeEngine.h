#ifndef FC_PLOTTER_GCODE_ENGINE_H
#define FC_PLOTTER_GCODE_ENGINE_H

#include <QObject>
#include <QStringList>

#include "FCSVGImageContainer.h"
#include "FCGCodeController.h"
#include "FCPumpRamp.h"

/**
 * @brief Компонент генерации и выполнения G-кода
 */
class FCPlotterGCodeEngine 
    : public QObject
{
Q_OBJECT
public:
    explicit FCPlotterGCodeEngine(QObject *parent = nullptr);
    
    void setHardware(FCGCodeController *controller, FCPumpRamp *ramp);
    
    [[nodiscard]] QStringList generate(const FCSVGImageContainer &container);
    [[nodiscard]] bool execute(const QString &command);
    
    [[nodiscard]] int estimateCurrentLayer(int commandIndex, int totalCommands, int totalLayers) const;

signals:
    void gcodeGenerated(int commandCount);
    void commandExecuted(int index, bool success);
    void progressUpdated(int percent, int layer);
    void executionError(const QString &command, const QString &error);

private:
    FCGCodeController *_controller = nullptr;
    FCPumpRamp *_ramp = nullptr;
};

#endif // FC_PLOTTER_GCODE_ENGINE_H
