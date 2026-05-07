#include <QThread>
#include <QDebug>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "FCI2CBus.h"

FCI2CBus::FCI2CBus(const QString &path, QObject *parent)
    : QObject(parent),
      _timeout{500},
      _file{path}
{
    open();
}

FCI2CBus::~FCI2CBus()
{
    if(_file.isOpen())
    {
        close();
    }
}

bool FCI2CBus::open()
{
    QMutexLocker locker(&_mutex);

    // Если файл уже открыт — закрываем и открываем заново
    if(isOpen())
    {
        _file.close();
    }

    // Открываем файл шины в режиме чтения/записи
    return _file.open(QIODevice::ReadWrite);
}

void FCI2CBus::close()
{
    QMutexLocker locker(&_mutex);

    if (isOpen())
    {
        _file.close();
    }
}

// ─────────────────────────────────────────────────────────────
// 📝 writeBytes: базовая запись данных на I2C-устройство
// ─────────────────────────────────────────────────────────────
bool FCI2CBus::writeBytes(uint8_t address, const QByteArray &data, FCI2CFlag flag)
{
    QMutexLocker locker(&_mutex);
    if(!_file.isOpen())
    {
        qDebug() << "Device not open";
        return false;
    }

    if(flag == FCI2CFlag::VerboseLog)
    {
        qDebug()
            << "[I2C TX] addr=0x" << Qt::hex << uint(address)
            << " len=" << Qt::dec << data.size()
            << " data=" << data.toHex().toUpper();
    }

    // Установка адреса ведомого (7-битный, бит R/W добавит драйвер)
    if(ioctl(_file.handle(), I2C_SLAVE, address) < 0)
    {
        if(flag == FCI2CFlag::IgnoreNACK && errno == ENXIO)
        {
            qDebug() << "Slave NACK ignored (IgnoreNACK flag)";
            return true;
        }
        qDebug() << "ioctl(I2C_SLAVE) failed:" << strerror(errno);
        return false;
    }

    int attempts = flag == FCI2CFlag::AutoRetry ? 3 : 1;
    while(attempts-- > 0)
    {
        qint64 written = ::write(_file.handle(), data.constData(), data.size());

        if(written == data.size())
        {
            return true;  // Успех
        }

        if(written < 0)
        {
            qDebug() << "write() error:" << strerror(errno);
        }
        else
        {
            qDebug() << "Partial write:" << written << "/" << data.size();
        }

        if(attempts > 0)
        {
            QThread::usleep(_timeout);  // Пауза перед повтором
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
// 📖 readBytes: базовое чтение данных с I2C-устройства
// ─────────────────────────────────────────────────────────────
QByteArray FCI2CBus::readBytes(uint8_t address, int count, FCI2CFlag flag)
{
    QMutexLocker locker(&_mutex);
    if(!_file.isOpen() || count <= 0)
    {
        qDebug() << "Invalid read request: open=" << _file.isOpen() << "count=" << count;
        return QByteArray();
    }

    if(flag == FCI2CFlag::VerboseLog)
    {
        qDebug()
            << "[I2C RX] addr=0x" << Qt::hex << uint(address)
            << " count=" << Qt::dec << count;
    }

    if(ioctl(_file.handle(), I2C_SLAVE, address) < 0)
    {
        qDebug() << "ioctl(I2C_SLAVE) failed:" << strerror(errno);
        return QByteArray();
    }

    QByteArray buffer(count, 0);
    int attempts = flag == FCI2CFlag::AutoRetry ? 3 : 1;

    while(attempts-- > 0)
    {
        qint64 bytesRead = ::read(_file.handle(), buffer.data(), count);

        if(bytesRead == count)
        {
            if(flag == FCI2CFlag::VerboseLog)
            {
                qDebug() << "[I2C RX] data=" << buffer.toHex().toUpper();
            }
            return buffer;  // Успех
        }

        if(bytesRead < 0)
        {
            qDebug() << "read() error:" << strerror(errno);
        }
        else
        {
            qDebug() << "Partial read:" << bytesRead << "/" << count;
        }

        if(attempts > 0)
        {
            QThread::usleep(_timeout);
        }
    }
    return QByteArray();  // Ошибка: пустой массив
}

// ─────────────────────────────────────────────────────────────
// 🔁 writeRead: атомарная запись+чтение с Repeated Start
// Критично для чтения регистров: между write и read НЕТ условия STOP
// ─────────────────────────────────────────────────────────────
QByteArray FCI2CBus::writeRead(uint8_t address, const QByteArray &data, int length, FCI2CFlag flag)
{
    QMutexLocker locker(&_mutex);
    if(!_file.isOpen() || length <= 0)
    {
        qDebug() << "Invalid writeRead request";
        return QByteArray{};
    }

    if(flag == FCI2CFlag::VerboseLog)
    {
        qDebug()
            << "[I2C WR] addr=0x" << Qt::hex << uint(address)
            << " write=" << data.toHex().toUpper()
            << " readCount=" << Qt::dec << length;
    }

    // Готовим структуру для ioctl(I2C_RDWR)
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;

    // Сообщение 1: ЗАПИСЬ (адрес регистра или команда)
    msgs[0].addr  = address;
    msgs[0].flags = 0;  // I2C_M_RD не установлен = WRITE
    msgs[0].len   = data.size();
    msgs[0].buf   = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data.constData()));

    // Сообщение 2: ЧТЕНИЕ (ответ от устройства)
    QByteArray readBuffer(length, 0);
    msgs[1].addr  = address;
    msgs[1].flags = I2C_M_RD;  // Флаг чтения
    msgs[1].len   = length;
    msgs[1].buf   = reinterpret_cast<uint8_t*>(readBuffer.data());

    rdwr.msgs  = msgs;
    rdwr.nmsgs = 2;

    int attempts = flag == FCI2CFlag::AutoRetry ? 3 : 1;

    while(attempts-- > 0)
    {
        // I2C_RDWR выполняет обе операции атомарно с Repeated Start
        if(ioctl(_file.handle(), I2C_RDWR, &rdwr) >= 0)
        {
            if(flag == FCI2CFlag::VerboseLog)
            {
                qDebug() << "[I2C WR] response=" << readBuffer.toHex().toUpper();
            }
            return readBuffer;  // Успех
        }

        qDebug() << "ioctl(I2C_RDWR) failed:" << strerror(errno);

        if(attempts > 0)
        {
            QThread::usleep(_timeout);
        }
    }

    return QByteArray();  // Ошибка: пустой массив
}

FCI2CDeviceAddressList FCI2CBus::scan(FCI2CFlag flag)
{
    QMutexLocker locker(&_mutex);
    FCI2CDeviceAddressList devices;

    if(!_file.isOpen())
    {
        if(flag == FCI2CFlag::VerboseLog)
        {
            qDebug() << "I2C scan aborted: device not open";
        }
        return devices;
    }

    if(flag == FCI2CFlag::VerboseLog)
    {
        qDebug() << "Starting I2C scan on" << _file.fileName()
                        << "range=0x03-0x77 timeout=" << _timeout << "ms";
    }

    // Стандартный диапазон 7-битных адресов (спецификация I2C)
    // 0x00-0x02 и 0x78-0x7F — зарезервированы, не сканируем
    for(uint8_t addr = 0x03; addr <= 0x77; ++addr)
    {
        // ─────────────────────────────────────────────────────
        // Шаг 1: Быстрая проверка через ioctl (ACK на адрес)
        // ─────────────────────────────────────────────────────
        bool deviceAcked = false;

        if(ioctl(_file.handle(), I2C_SLAVE, addr) >= 0)
        {
            // Адрес принят — устройство ответило ACK
            deviceAcked = true;
        }
        else
        {
            // Ошибка ioctl: устройство не ответило или шина занята
            // При AutoRetry можно попробовать ещё раз
            if(flag == FCI2CFlag::AutoRetry)
            {
                QThread::usleep(200);
                if(ioctl(_file.handle(), I2C_SLAVE, addr) >= 0)
                {
                    deviceAcked = true;
                }
            }
        }

        if(!deviceAcked)
        {
            continue; // Адрес свободен — переходим к следующему
        }

        // ─────────────────────────────────────────────────────
        // Шаг 2: Дополнительная проверка чтением (опционально)
        // Некоторые устройства могут "висеть" на шине, но не отдавать данные.
        // Пробуем прочитать 1 байт в неблокирующем режиме.
        // ─────────────────────────────────────────────────────
        bool deviceResponds = true; // По умолчанию считаем, что устройство валидно

        // Сохраняем текущие флаги дескриптора, чтобы не сломать неблокирующий режим
        int origFlags = fcntl(_file.handle(), F_GETFL, 0);

        // Временно ставим O_NONBLOCK, чтобы read() не завис, если устройство молчит
        fcntl(_file.handle(), F_SETFL, origFlags | O_NONBLOCK);

        char dummy;
        qint64 result = ::read(_file.handle(), &dummy, 1);

        // Восстанавливаем флаги
        fcntl(_file.handle(), F_SETFL, origFlags);

        // Если read вернул ошибку EIO/ENXIO — устройство, скорее всего, "призрачное"
        if(result < 0 && (errno == EIO || errno == ENXIO || errno == EREMOTEIO))
        {
            deviceResponds = false;
        }
        // result == 0 или > 0 — устройство реально ответило данными

        // Если включена строгая проверка и устройство не ответило на read — пропускаем
        if(!deviceResponds)
        {
            if(flag == FCI2CFlag::VerboseLog)
            {
                qDebug()
                    << "[I2C SCAN] 0x" << Qt::hex << uint(addr)
                    << ": ACKed but no data (phantom?)";
            }
            continue;
        }

        // ─────────────────────────────────────────────────────
        // Устройство найдено — добавляем в список
        // ─────────────────────────────────────────────────────
        devices.append(addr);

        if (flag == FCI2CFlag::VerboseLog)
        {
            qDebug() << "[I2C SCAN] FOUND: 0x" << Qt::hex << uint(addr);
        }

        // Задержка между опросами для стабильности шины
        if (_timeout > 0) {
            QThread::msleep(static_cast<ulong>(_timeout));
        }
    }

    if (flag == FCI2CFlag::VerboseLog)
    {
        qDebug() << "I2C scan complete: found" << devices.size() << "device(s)";
    }

    return devices;
}
