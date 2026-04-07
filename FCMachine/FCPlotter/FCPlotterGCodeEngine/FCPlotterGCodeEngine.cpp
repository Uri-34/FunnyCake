#include <QDebug>

#include "FCPlotterGCodeEngine.h"
#include "FCState.h"

FCPlotterGCodeEngine::FCPlotterGCodeEngine(QObject *parent)
    : QObject(parent)
{
}

void FCPlotterGCodeEngine::setHardware(FCMarlinController *controller, 
                                        FCPumpRamp *ramp)
{
    _controller = controller;
    _ramp = ramp;
}

QStringList FCPlotterGCodeEngine::generate(const FCSVGImageContainer &container)
{
    QStringList gcode;
    
    // Начальная инициализация
    gcode << "G21 ";          // Миллиметры
    gcode << "G90 ";          // Абсолютные координаты
    gcode << "M82 ";          // Абсолютная подача экструдера
    gcode << "G28 X0 Y0 Z0 "; // Домашняя позиция
    gcode << "G1 Z5 F5000 ";  // Подъём головки
    gcode << "M107 ";         // Выключить насос

    for (int layerIdx = 0; layerIdx < static_cast<int>(container.layerCount()); ++layerIdx) {
        const auto &layer = container.layer(layerIdx);

        gcode << QString("; Layer %1 ").arg(layerIdx + 1);
        gcode << "M106 S255 ";  // Включить насос

        for (const auto &figure : layer.figures) {
            // Перемещение к начальной точке БЕЗ подачи краски
            if (!figure.points.isEmpty()) {
                const auto &startPoint = figure.points.first();
                gcode << QString("G1 X%1 Y%2 Z%3 F3000 M107 ")
                    .arg(startPoint.x(), 0, 'f', 3)
                    .arg(startPoint.y(), 0, 'f', 3)
                    .arg(layer.zPosition, 0, 'f', 3);
            }

            gcode << "M106 S200 ";  // Включение подачи

            for (int i = 1; i < figure.points.size(); ++i) {
                const auto &point = figure.points.at(i);
                gcode << QString("G1 X%1 Y%2 Z%3 F%4 ")
                    .arg(point.x(), 0, 'f', 3)
                    .arg(point.y(), 0, 'f', 3)
                    .arg(layer.zPosition, 0, 'f', 3)
                    .arg(5000);
            }

            gcode << "M107 ";  // Выключение подачи
        }

        gcode << QString("G1 Z%1 F2000 ").arg(layer.zPosition + 5.0);
        gcode << "M107 ";
    }

    // Финальные команды
    gcode << "G1 Z10 F5000 ";
    gcode << "G28 X0 Y0 ";
    gcode << "M107 ";
    gcode << "M84 ";

    emit gcodeGenerated(gcode.size());
    return gcode;
}

bool FCPlotterGCodeEngine::execute(const QString &command)
{
    if (command.trimmed().isEmpty() || command.startsWith(';'))
    {
        return true;
    }

    if (!_controller) {
        emit executionError(command, QStringLiteral("Контроллер не инициализирован"));
        return false;
    }

    if (!_controller->sendCommand(command.toUtf8())) {
        emit executionError(command, QStringLiteral("Не удалось отправить команду"));
        return false;
    }

    if (!_controller->is(FCReadyState::Ready)) {
        emit executionError(command, QStringLiteral("Потеряно соединение"));
        return false;
    }

    emit commandExecuted(0, true);
    return true;
}

int FCPlotterGCodeEngine::estimateCurrentLayer(int commandIndex, 
                                                int totalCommands, 
                                                int totalLayers) const
{
    if (totalLayers <= 0 || totalCommands <= 0) {
        return -1;
    }
    double commandsPerLayer = static_cast<double>(totalCommands) / totalLayers;
    return static_cast<int>(commandIndex / commandsPerLayer);
}
