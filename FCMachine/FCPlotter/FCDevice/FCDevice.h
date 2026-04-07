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
 * @details Состояние состоит строго из 2 компонентов: FCReadyState, FCErrorType.
 *          Прямая модификация _state запрещена. Используйте protected set().
 * @see FCStateT, FCSerialDevice, FCI2CDevice
 */
class FCDevice
    : public QObject
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCDevice)
public:
    /// Состояние по умолчанию: NotReady + NoError
    inline static const FCDeviceState FCDeviceDefaultState {FCReadyState::NotReady, FCErrorType::None};

    explicit FCDevice(const QString &name, QObject *parent = nullptr);
    ~FCDevice() override = default;

    [[nodiscard]] virtual QString securityCode(int timeoutMs) = 0;

    /// Безопасный доступ к состоянию (только чтение)
    [[nodiscard]] inline FCDeviceState& state() noexcept { return _state; }

//protected:
//    /// Инициализация устройства (переопределяется в потомках)
//    virtual bool init() = 0;
//    /// Очистка ресурсов перед удалением
//    virtual bool final() = 0;

signals:
    /// Единственный сигнал изменения состояния
    void condition(const FCDeviceState &state, const QString &details = QString());

private:
    FCDeviceState _state; ///< Внутреннее хранилище состояния (2 компонента)
};

using FCDeviceList = QList<FCDevice *>;
using FCDeviceQList = QList<QPointer<FCDevice>>;

static_assert(std::has_virtual_destructor_v<FCDevice>,
              "FCDevice должен иметь виртуальный деструктор для безопасного полиморфного удаления");

#endif // FC_DEVICE_H
