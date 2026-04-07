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

QByteArray FCI2CBus::readBytes(uint8_t address, int count, int flags)
{
    Q_UNUSED(flags);
    QMutexLocker locker(&_mutex);

    // Проверка открытия шины
    if(!isOpen())
    {
        return QByteArray();
    }

    // Установка адреса устройства через ioctl
    if(ioctl(_file.handle(), I2C_SLAVE, address) < 0)
    {
        return QByteArray();
    }

    // Чтение данных
    QByteArray buffer(count, '\0');
    qint64 bytesRead = ::read(_file.handle(), buffer.data(), count);

    if(bytesRead < 0 || bytesRead != count)
    {
        return QByteArray();
    }

    // Успешное чтение — сброс ошибки
    return buffer;
}

bool FCI2CBus::writeBytes(uint8_t address, const QByteArray &data, int flags)
{
    Q_UNUSED(flags);
    QMutexLocker locker(&_mutex);

    if(!isOpen())
    {
        return false;
    }

    if(ioctl(_file.handle(), I2C_SLAVE, address) < 0)
    {
        return false;
    }

    qint64 bytesWritten = ::write(_file.handle(), data.constData(), data.size());
    if(bytesWritten < 0 || bytesWritten != data.size())
    {
        return false;
    }

    return true;
}

QByteArray FCI2CBus::writeRead(uint8_t address, const QByteArray &writeData, int count, int flags)
{
    QMutexLocker locker(&_mutex);

    if(!isOpen())
    {
        return QByteArray();
    }

    // Запись данных (адрес регистра)
    if(!writeBytes(address, writeData, flags))
    {
        // writeBytes уже установил состояние ошибки
        return QByteArray();
    }

    // Чтение данных
    // readBytes() уже установил состояние ошибки
    return readBytes(address, count, flags);
}

uint8_t FCI2CBus::readByte(uint8_t address)
{
    QByteArray data = readBytes(address, 1);
    return data.isEmpty() ? 0 : static_cast<uint8_t>(data[0]);
}

bool FCI2CBus::writeByte(uint8_t address, uint8_t byte)
{
    QByteArray data(1, static_cast<char>(byte));
    return writeBytes(address, data);
}

uint8_t FCI2CBus::readRegister(uint8_t address, uint8_t reg)
{
    // Сначала записываем адрес регистра
    if(!writeByte(address, reg))
    {
        // writeByte уже установил состояние ошибки
        return 0;
    }

    // Затем читаем 1 байт данных
    return readByte(address);
}

bool FCI2CBus::writeRegister(uint8_t address, uint8_t reg, uint8_t value)
{
    QByteArray data;
    data.append(static_cast<char>(reg));
    data.append(static_cast<char>(value));

    // writeByte уже установил состояние ошибки
    return writeBytes(address, data);
}

FCI2CDeviceAddressList FCI2CBus::scan(int timeOutMs)
{
    QMutexLocker locker(&_mutex);
    FCI2CDeviceAddressList devices;

    // Проверка открытия шины
    if(!isOpen())
    {
        return devices;
    }

    // Сканирование стандартного диапазона адресов I2C (0x03–0x77)
    // Адреса 0x00–0x02 и 0x78–0x7F зарезервированы спецификацией
    for(uint8_t addr = 0x03; addr <= 0x77; ++addr)
    {
        // Попытка установить адрес устройства
//        if(ioctl(_file.handle(), I2C_SLAVE, addr) >= 0)
//        {
//            // Проверка наличия устройства: попытка чтения 1 байта
//            char testByte;
//            if(::read(_file.handle(), &testByte, 1) >= 0)
//            {
//                devices.append(addr);
//            }
//            // Игнорируем ошибки отдельных адресов — это нормально для свободных адресов

//        }

        if(readByte(addr))
        {
            devices.append(addr);
        }

        // Небольшая задержка между опросами для стабильности (1 мс)
        QThread::usleep(timeOutMs);
    }

    // Сканирование успешно завершено — НЕ устанавливаем состояние ошибки
    // (даже если найдено 0 устройств — это валидный результат)
    return devices;
}
