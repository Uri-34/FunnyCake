#ifndef FC_PLOTTER_TEST_SUITE_H
#define FC_PLOTTER_TEST_SUITE_H

#include <QObject>
#include "FCMarlinController.h"

#include "FCPumpRamp.h"

/**
 * @brief Компонент диагностических тестов плоттера
 */
class FCPlotterTestSuite 
    : public QObject
{
Q_OBJECT
public:
    explicit FCPlotterTestSuite(QObject *parent = nullptr);
    
    void setHardware(FCMarlinController *controller, FCPumpRamp *ramp);
    
    [[nodiscard]] bool runShortTest();
    [[nodiscard]] bool runLongTest();
    [[nodiscard]] bool runGridTest(int cols, int rows);
    [[nodiscard]] bool runFlowTest();
    [[nodiscard]] bool runLoadTest();

signals:
    void testStarted(const QString &testName);
    void testProgress(const QString &testName, int step, int total);
    void testCompleted(const QString &testName, bool success, const QString &details);
    void testError(const QString &testName, const QString &error);

private:
    FCMarlinController *_controller = nullptr;
    FCPumpRamp *_ramp = nullptr;
    
    [[nodiscard]] bool executeCommand(const QString &command);
};

#endif // FC_PLOTTER_TEST_SUITE_H
