#ifndef FC_HEAD_H
#define FC_HEAD_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QCanBus>

#include "FC3DPoint.h"
#include "FCCanDevice.h"
#include "FCState.h"
#include "FCRange.h"

/**
 * @brief Коммуникационный класс, осуществляет общение с прошивкой печатающей головки по I2C.
 *
 * Отправляет и получает данные печатающей головки :
 *   - Количество фидеров в головке.
 *   - Координаты головки на поле рисования (x, y, z) (мм),
 *   - Секретный код из контроллера головки для обеспечения безопасности
 *   - Состояние головки (state).
 */
class FCCanHead
    : protected FCCanDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCCanHead)
public:
    QByteArray SecurityCode = {"fldskfks;lfk;sl"};

    enum FCCanHeadCommand
    {
        GetState = 0x01,

        GetFeedersCount = 0x10,
        FeedersToHome = 0x11,
        SwitchToFeeder = 0x12,

        HeadToHome = 0x20,
        HeadToWork = 0x21,

        GetSecurityCode = 0x30
    };

    explicit FCCanHead(QObject *parent = nullptr)
        : FCCanDevice("can0", parent)
    {}

    ~FCCanHead() override = default;

    /// получение серретного кода
    inline bool isSecretCheck() { return exchange(GetSecurityCode) == SecurityCode; }

public slots:
    /// отправить все фидеры в верхнее положение
    inline void feedersToHomePosition() { send(FeedersToHome); }
    /// переключить на конкретный фидер
    void switchToFeeder(uint8_t number) { send(SwitchToFeeder, QByteArray{1, static_cast<uint8_t>(FCRange<uint8_t>(0, _feedersCount).clamped(number))}); }
    /// переместить головку в парковочное положение
    inline void headToHomePosition() { send(HeadToHome); }
    /// переместить головку в рабочее положение ()
    inline void headToWorkPosition() { send(HeadToWork); }

private:
    /// инициализация обьекта
    bool init();
    /// количество фидеров в головке
    inline uint8_t feedersCount() { return static_cast<uint8_t>(exchange(GetFeedersCount).at(0)); }

    /// количество фидеров (получено от контроллера гоовки)
    uint8_t _feedersCount;
};

#endif // FC_HEAD_H
