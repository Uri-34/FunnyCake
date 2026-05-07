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
    explicit FCSerialDevice(const QString &portName, QObject *parent = nullptr);
    ~FCSerialDevice() override;

    bool send(const QString &command, int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES);
    bool send(const QStringList &commands, int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES);

    [[nodiscard]] const QString &receive() const noexcept { return _answer; }

    inline bool readiness() override { return state().is(FCOpenState::Open, FCReadyState::Ready) &&
                                              portAvailable() && _port->isOpen() &&
                                              !objectName().isEmpty();
                                     }
protected:
    bool init();
    bool final();

    void clear();

    static constexpr int MAX_RETRIES = 3;
    static constexpr int DEFAULT_TIMEOUT_MS = 1000;

private:
    mutable QMutex _portMutex;

    bool writeBytesRaw(const QString &bytes, int timeoutMs);
    bool portAvailable();

    QSerialPort *_port = nullptr;
    QByteArray _buffer;
    QString _answer;
};

using FCSerialDeviceList = QList<FCSerialDevice *>;

#endif // FC_SERIAL_DEVICE_H
