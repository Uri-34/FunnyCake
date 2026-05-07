#ifndef FC_PUMP_RAMP_H
#define FC_PUMP_RAMP_H

#include <QObject>
#include <QList>
#include <QColor>
#include <QTimer>
#include "FCI2CDevice.h"
#include "FCLM75AThermometer.h"

/**
 * @brief Контроллер насосной рампы с датчиками температуры.
 * @details Управляет 3+ насосами по цвету, мониторит температуру через LM75A.
 *          Гарантирует отключение всех насосов перед включением нового.
 *          Периодически опрашивает датчики и эмитит signal temperatureChanged.
 */
class FCPumpRamp
    : public FCI2CDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCPumpRamp)
public:
    explicit FCPumpRamp(FCI2CBus *bus, QObject *parent = nullptr);
    ~FCPumpRamp() override;

    /// Чтение температуры с термометра по номеру
    qreal thermometer(int number) const;

    /// Заглушка для совместимости с интерфейсом
    QString securityCode(int timeoutMs) { Q_UNUSED(timeoutMs); return {}; }

    /// Проверка на соответствие секретному ключу
    inline bool isSecretCheck()
    {
        // логика проверки секретного кода, если это будет реализовано на плате
        return true;
    }

    /// Переключение на конкретный насос
    bool switchTo(uint8_t pumpNumber);

public slots:
    void onStart(const QString &model = QString()) override;
    void onStop() override;
    void onPause() override;
    void onTest() override;

signals:
    /// Сигнал: насос переключен
    void pumpSwitched(uint8_t pumpNumber);
    /// Сигнал: изменены данные температуры (эмитится по таймеру)
    void temperatureChanged(QList<float> temperatures);

protected:
    bool init();
    bool final();

    /// очистка шины (вспомогательный метод)
    inline bool clear()
    {
        return send({static_cast<char>(0xFF), static_cast<char>(0xFF)});
    }

private slots:
    /// Обработчик таймера: опрос температур и эмиссия сигнала при изменении
    void onTemperaturePoll();

private:
    const QList<uint8_t> _pumps;              ///< Номера управляемых насосов
    QList<float> _lastTemperatures;           ///< Кэш последних значений для сравнения
    QTimer _tempPollTimer;                    ///< Таймер периодического опроса (значение, не указатель)
};

#endif // FC_PUMP_RAMP_H
