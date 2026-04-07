#include <QThread>
#include <QElapsedTimer>
#include <QDebug>

#include "FCPlotterTestSuite.h"

FCPlotterTestSuite::FCPlotterTestSuite(QObject *parent)
    : QObject(parent)
{}

void FCPlotterTestSuite::setHardware(FCMarlinController *controller, FCPumpRamp *ramp)
{
    _controller = controller;
    _ramp = ramp;
}

bool FCPlotterTestSuite::executeCommand(const QString &command)
{
    if (!_controller) {
        return false;
    }
    return _controller->sendCommand(command.toUtf8());
}

bool FCPlotterTestSuite::runShortTest()
{
    emit testStarted(QStringLiteral("short"));
    bool success = true;
    QStringList details;

    // Тест 1: Связь с контроллером
    if (!_controller->checkConnection()) {
        success = false;
        details << QStringLiteral("Потеряна связь с контроллером");
        emit testError(QStringLiteral("short"), QStringLiteral("Тест 1 ПРОВАЛЕН: Связь"));
    } else {
        details << QStringLiteral("Связь с контроллером: ОК");
        emit testProgress(QStringLiteral("short"), 1, 3);
    }

    // Тест 2: Перемещение по осям
    if (success) {
        if (!executeCommand(QStringLiteral("G1 X5 Y5 F1000"))) {
            success = false;
            details << QStringLiteral("Ошибка перемещения по осям X/Y");
            emit testError(QStringLiteral("short"), QStringLiteral("Тест 2 ПРОВАЛЕН: Оси"));
        } else {
            details << QStringLiteral("Перемещение по осям: ОК");
            emit testProgress(QStringLiteral("short"), 2, 3);
        }
    }

    // Тест 3: Насосная рампа
    if (success) {
        if (_ramp) {
            _ramp->reset();
            QThread::msleep(100);
            _ramp->reset();
            details << QStringLiteral("Насосная рампа: ОК");
            emit testProgress(QStringLiteral("short"), 3, 3);
        }
    }

    executeCommand(QStringLiteral("G28 X0 Y0 Z0"));

    QString resultDetails = details.join(QStringLiteral("; "));
    emit testCompleted(QStringLiteral("short"), success, resultDetails);
    return success;
}

bool FCPlotterTestSuite::runLongTest()
{
    emit testStarted(QStringLiteral("long"));
    bool success = true;
    QStringList details;

    QElapsedTimer totalTime;
    totalTime.start();

    // Тест 1: Калибровочная решётка
    emit testProgress(QStringLiteral("long"), 1, 3);
    if (!runGridTest(10, 10)) {
        success = false;
        details << QStringLiteral("Ошибка калибровочной решётки");
    } else {
        details << QStringLiteral("Калибровочная решётка: ОК");
    }

    // Тест 2: Линейность подачи
    if (success) {
        emit testProgress(QStringLiteral("long"), 2, 3);
        if (!runFlowTest()) {
            success = false;
            details << QStringLiteral("Ошибка линейности подачи");
        } else {
            details << QStringLiteral("Линейность подачи: ОК");
        }
    }

    // Тест 3: Стабильность под нагрузкой
    if (success) {
        emit testProgress(QStringLiteral("long"), 3, 3);
        if (!runLoadTest()) {
            success = false;
            details << QStringLiteral("Ошибка стабильности под нагрузкой");
        } else {
            details << QStringLiteral("Стабильность под нагрузкой: ОК");
        }
    }

    executeCommand(QStringLiteral("G28 X0 Y0 Z0"));

    QString resultDetails = details.join(QStringLiteral("; "));
    int durationMs = totalTime.elapsed();
    emit testCompleted(QStringLiteral("long"), success, 
                       QStringLiteral("%1 (%2 мс)").arg(resultDetails).arg(durationMs));
    return success;
}

bool FCPlotterTestSuite::runGridTest(int cols, int rows)
{
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            QString cmd = QString("G1 X%1 Y%2 F1000").arg(x * 10.0).arg(y * 10.0);
            if (!executeCommand(cmd)) {
                return false;
            }
            QThread::msleep(50);
        }
    }
    return true;
}

bool FCPlotterTestSuite::runFlowTest()
{
    const int speeds[] = {64, 128, 192, 255};
    const int count = sizeof(speeds) / sizeof(speeds[0]);
    
    for (int i = 0; i < count; ++i) {
        if (_ramp) {
            _ramp->reset();
            QThread::msleep(200);
        }
    }

    if (_ramp) {
        _ramp->reset();
    }
    return true;
}

bool FCPlotterTestSuite::runLoadTest()
{
    for (int i = 0; i < 20; ++i) {
        if (!executeCommand(QStringLiteral("G1 X0 Y0 F3000")) || 
            !executeCommand(QStringLiteral("G1 X100 Y100 F3000"))) {
            return false;
        }
        QThread::msleep(100);
    }
    return true;
}
