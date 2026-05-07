#ifndef FC_STATE_TYPE_H
#define FC_STATE_TYPE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QPointer>
#include "FCState.h"

/**
 * @brief Шаблонный базовый класс-контроллер состояния.
 * @details Q_OBJECT не используется здесь, так как MOC не обрабатывает шаблоны.
 *          Конкретные классы-наследники добавят Q_OBJECT самостоятельно.
 */
template <typename StateType>
class FCStateType : public QObject
{
public:
    using State = StateType;

    explicit FCStateType(const QString &name, QObject *parent = nullptr)
        : QObject(parent), _name(name) {}

    ~FCStateType() override = default;
    Q_DISABLE_COPY_MOVE(FCStateType)

    /// Безопасный доступ к состоянию
    inline StateType& state() noexcept { return _state; }
    inline const StateType& state() const noexcept { return _state; }

    /// Готовность устройства/сервиса к работе. Переопределяется в наследниках при необходимости.
    virtual bool readiness() {
        // Базовая логика. В оригинале использовалась и для Service, и для Device.
        return _state.is(FCOpenState::Open, FCReadyState::Ready);
    }

signals:
    /// Единый сигнал изменения состояния. Тип параметра выводится из шаблона.
    void condition(const StateType &state, QObject *object, const QString &message = QString());

protected:
    QString _name;
    StateType _state;
};

#endif // FC_STATE_TYPE_H
