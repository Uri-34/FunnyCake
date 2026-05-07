#ifndef FC_SERVICE_H
#define FC_SERVICE_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QPointer>
#include <QString>

#include "FCStateType.h"

//class FCService
//    : public QObject
//{
//Q_OBJECT
//Q_DISABLE_COPY_MOVE(FCService)
//public:
//    /// @brief состояние устройства (стандартный набор для логики)
//    using FCServiceState = FCStateT<FCReadyState, FCPlayState, FCErrorType>;
//
//    explicit FCService(const QString &name, QObject *parent = nullptr);
//    ~FCService() override = default;
//
//    /// безопасный доступ к состоянию (только чтение)
//    inline FCServiceState& state() noexcept { return _state; }
//
//    /// готовность устройства к работе (определяется в классе наследнике)
//    virtual bool readiness() { return _state.is(FCOpenState::Open, FCReadyState::Ready); }
//
//signals:
//    /// единный сигнал изменения состояния
//    void condition(const FCService::FCServiceState &state, QObject *object, const QString &message = QString());
//
//private:
//    ///< внутреннее хранилище состояния
//    FCServiceState _state;
//};

class FCService
    : public FCStateType<FCStateT<FCReadyState, FCPlayState, FCErrorType>>
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCService)
public:
    /// Алиас для обратной совместимости с кодом, где использовался FCService::FCServiceState
    using FCServiceState = State; 

    explicit FCService(const QString &name, QObject *parent = nullptr)
        : FCStateType(name, parent) {}
    ~FCService() override = default;
};

using FCServiceList = QList<FCService *>;

#endif // FC_SERVICE_H
