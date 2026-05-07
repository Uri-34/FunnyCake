#ifndef FC_DEVICE_H
#define FC_DEVICE_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QPointer>
#include <QString>

#include "FCStateType.h"

/**
 * @brief Абстрактный базовый класс для управления аппаратными устройствами.
 * @details Состояние состоит строго из 3 компонентов: FCReadyState, FCPlayState::Stop, FCErrorType.
 *          Прямая модификация _state запрещена..
 */
//class FCDevice
//    : public QObject
//{
//Q_OBJECT
//Q_DISABLE_COPY_MOVE(FCDevice)
//public:
//    /// @brief состояние устройства (стандартный набор для логики)
//    using FCDeviceState = FCStateT<FCOpenState, FCReadyState, FCPlayState, FCErrorType>;
//
//    explicit FCDevice(const QString &name, QObject *parent = nullptr);
//    ~FCDevice() override = default;
//
//    /// безопасный доступ к состоянию (только чтение)
//    inline FCDeviceState& state() noexcept { return _state; }
//
//    /// готовность устройства к работе (определяется в классе наследнике)
//    virtual bool readiness() { return _state.is(FCOpenState::Open, FCReadyState::Ready); }
//
//signals:
//    /// единный сигнал изменения состояния
//    void condition(const FCDevice::FCDeviceState &state, QObject *object, const QString &message = QString());
//
//private:
//    ///< внутреннее хранилище состояния
//    FCDeviceState _state;
//};

class FCDevice 
    : public FCStateType<FCStateT<FCOpenState, FCReadyState, FCPlayState, FCErrorType>>
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCDevice)
public:
    /// Алиас для обратной совместимости с кодом, где использовался FCDevice::FCDeviceState
    using FCDeviceState = State; 

    explicit FCDevice(const QString &name, QObject *parent = nullptr)
        : FCStateType(name, parent)
    {}
    ~FCDevice() override = default;

public slots:
    virtual void onStart(const QString &model) = 0;
    virtual void onStop() = 0;
    virtual void onPause()  = 0;

//    virtual void onReset() = 0;

    virtual void onTest() = 0;

signals:
    void started(const QString &text, QObject *object = nullptr);
    void stoped(const QString &text, QObject *object = nullptr);
    void paused(const QString &text, QObject *object = nullptr);
//    void reseted(const QString &text, QObject *object = nullptr);

    void message(const QString &text, QObject *object = nullptr);
    void error(const QString &text, QObject *object = nullptr);

    void progress(int percent, QObject *object = nullptr);

    void test(const QString &testName, bool success, const QString &details);
};

using FCDeviceList = QList<FCDevice *>;

#endif // FC_DEVICE_H
