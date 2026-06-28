#ifndef FC_DEVICE_H
#define FC_DEVICE_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QPointer>
#include <QString>

#include "FCState.h"

/**
 * @brief Абстрактный базовый класс для управления аппаратными устройствами.
 * @details Состояние состоит строго из компонентов: FCOpenState, FCReadyState, FCPlayState, FCErrorTypee.
 *          Прямая модификация _state запрещена..
 */

class FCDevice 
    : public QObject
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCDevice)
public:
    /// @brief состояние устройства (стандартный набор для FCDeviceState)
    using FCDeviceState = FCStateT<FCOpenState, FCReadyState, FCPlayState, FCErrorType>;

    explicit FCDevice(const QString &name, QObject *parent = nullptr)
        : QObject(parent)
    {
        setObjectName(name);
    }

    ~FCDevice() override = default;

    inline FCDeviceState& state() { return _state; };

protected:
    virtual bool init() = 0;
    virtual bool final() = 0;

public slots:
    virtual bool onStart() = 0;
    virtual bool onStop() = 0;
    virtual bool onPause()  = 0;

    virtual bool onReset() = 0;
    virtual bool onTest() = 0;

signals:
    void condition(const FCDevice::FCDeviceState &state, QObject *object, const QString &msg = QString());

    void started(const QString &msg, QObject *object = nullptr);
    void stoped(const QString &msg, QObject *object = nullptr);
    void paused(const QString &msg, QObject *object = nullptr);

    void reseted(const QString &msg, QObject *object = nullptr);

    void message(const QString &msg, QObject *object = nullptr);
    void error(const QString &msg, QObject *object = nullptr);

    void progress(int percent, QObject *object = nullptr);

    void test(bool success, const QString &msg);

private:
    /// @brief состояние устройства
    FCDeviceState _state;
};

using FCDeviceList = QList<FCDevice*>;

#endif // FC_DEVICE_H
