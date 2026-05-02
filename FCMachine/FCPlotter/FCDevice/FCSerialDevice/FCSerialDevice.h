#ifndef FC_SERIAL_DEVICE_H
#define FC_SERIAL_DEVICE_H

#include <QMutex>
#include <QSerialPort>
#include <QByteArray>
#include <QList>
#include <QPointer>
#include <QString>
#include "FCDevice.h"


class FCSerialDevice
    : public FCDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCSerialDevice)
public:
    // ========================================================================
    // КОНСТРУКТОРЫ И ДЕСТРУКТОР
    // ========================================================================
    explicit FCSerialDevice(const QString &portName, QObject *parent = nullptr);

    ~FCSerialDevice() override;

    // ========================================================================
    // ПУБЛИЧНЫЕ МЕТОДЫ ВВОДА-ВЫВОДА
    // ========================================================================

    bool send(const QString &command, int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES);

    bool send(const QStringList &commands, int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES);

    [[nodiscard]] const QString &answer() const noexcept { return _cachedAnswer; }

protected:
    // ========================================================================
    // ВИРТУАЛЬНЫЕ МЕТОДЫ ЖИЗНЕННОГО ЦИКЛА
    // ========================================================================
    bool init();

    bool final();

    // ========================================================================
    // КОНСТАНТЫ КЛАССА
    // ========================================================================

    static constexpr int MAX_RETRIES = 3;

    static constexpr int DEFAULT_TIMEOUT_MS = 1000;

    // ========================================================================
    // ЗАЩИЩЁННЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    // ========================================================================
private:
    // ========================================================================
    // ЗАКРЫТЫЕ МЕТОДЫ И ЧЛЕНЫ ДАННЫХ
    // ========================================================================
    [[nodiscard]] bool writeBytesRaw(const QString &bytes, int timeoutMs);

    void clear();

    [[nodiscard]] bool available() const;

    QSerialPort *_port = nullptr;

    QByteArray _buffer;

    QString _cachedAnswer;

    mutable QMutex _portMutex;
};

// ============================================================================
// ПСЕВДОНИМЫ ТИПОВ
// ============================================================================

using FCSerialDeviceList = QList<FCSerialDevice *>;

using FCSerialDeviceQList = QList<QPointer<FCSerialDevice>>;

#endif // FC_SERIAL_DEVICE_H
