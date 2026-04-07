#include "FCFileGuarder.h"

#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QCryptographicHash>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>

// FileParseGuard
FileParseGuard::FileParseGuard(const QString &filePath, bool useImmutable)
    : _filePath(filePath),
      _useImmutable(useImmutable),      // ← Сохраняем параметр
      _immutableSet(false),
      _registered(false),
      _locked(false),
      _fd(-1)
{
    lock();
}

FileParseGuard::~FileParseGuard()
{
    unlock();
}

void FileParseGuard::lock()
{
    if(_locked)
    {
        return;
    }

    // 1. Регистрируем в глобальном реестре (потокобезопасность)
    if(!FileLockRegistry::instance().acquire(_filePath))
    {
        qWarning() << "FileParseGuard: Файл уже заблокирован другим потоком:" << _filePath;
        return;
    }
    _registered = true;

    // 2. Создаём файл-метку с PID (для аварийного восстановления)
    createLockFile();

    // 3. Устанавливаем immutable (ТОЛЬКО если _useImmutable = true и мы root)
    if(_useImmutable && geteuid() == 0)
    {
        QProcess chattr;
        chattr.start("chattr", QStringList() << "+i" << _filePath);
        chattr.waitForFinished(5000);

        if(chattr.exitCode() == 0)
        {
            _immutableSet = true;
            qDebug() << "FileParseGuard: Установлен флаг immutable:" << _filePath;
        }
        else
        {
            qWarning() << "FileParseGuard: Не удалось установить chattr +i:" << chattr.readAllStandardError().trimmed();
        }
    }
    else if(_useImmutable && geteuid() != 0)
         {
            qWarning() << "FileParseGuard: Требуется root для chattr +i:" << _filePath;
         }

    // 4. Блокируем файл через flock (POSIX)
    _fd = open(_filePath.toStdString().c_str(), O_RDONLY);
    if(_fd == -1)
    {
        qWarning() << "FileParseGuard: Не удалось открыть файл:" << strerror(errno);
        return;
    }

    struct flock fl;
    fl.l_type = F_WRLCK;      // Эксклюзивная блокировка
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;             // До конца файла
    fl.l_pid = 0;

    if(fcntl(_fd, F_SETLK, &fl) == -1)
    {
        qWarning() << "FileParseGuard: Не удалось заблокировать файл (flock):" << strerror(errno);
        close(_fd);
        _fd = -1;
        return;
    }

    _locked = true;
    qDebug() << "FileParseGuard: Файл заблокирован:" << _filePath;
}

void FileParseGuard::unlock()
{
    if(!_locked && !_registered)
    {
        return;
    }

    // 1. Снимаем блокировку flock
    if(_fd != -1)
    {
        if(_locked)
        {
            struct flock fl;
            fl.l_type = F_UNLCK;
            fl.l_whence = SEEK_SET;
            fl.l_start = 0;
            fl.l_len = 0;
            fcntl(_fd, F_SETLK, &fl);
        }
        close(_fd);
        _fd = -1;
        _locked = false;
    }

    // 2. Снимаем immutable (ТОЛЬКО если устанавливали)
    if(_immutableSet && geteuid() == 0)
    {
        QProcess chattr;
        chattr.start("chattr", QStringList() << "-i" << _filePath);
        chattr.waitForFinished(5000);

        if(chattr.exitCode() == 0)
        {
            _immutableSet = false;
            qDebug() << "FileParseGuard: Флаг immutable снят:" << _filePath;
        }
        else
        {
            qCritical() << "FileParseGuard: КРИТИЧЕСКАЯ ОШИБКА! Не удалось снять chattr -i:" << chattr.readAllStandardError().trimmed();
        }
    }

    // 3. Удаляем файл-метку
    removeLockFile();

    // 4. Снимаем с регистрации в реестре
    if(_registered)
    {
        FileLockRegistry::instance().release(_filePath);
        _registered = false;
    }

    qDebug() << "FileParseGuard: Блокировка снята:" << _filePath;
}

void FileParseGuard::createLockFile()
{
    QDir lockDir("/var/run/fcfileguarder");
    if(!lockDir.exists())
    {
        if(!lockDir.mkpath("."))
        {
            qWarning() << "FileParseGuard: Не удалось создать директорию:" << lockDir.path();
            return;
        }
    }

    // Создаём уникальное имя файла на основе хеша пути
    QString hash = QCryptographicHash::hash(_filePath.toUtf8(), QCryptographicHash::Md5).toHex();

    _lockFilePath = lockDir.filePath(hash + ".lock");

    QFile pidFile(_lockFilePath);
    if(pidFile.open(QIODevice::WriteOnly))
    {
        pidFile.write(QByteArray::number(getpid()));
        pidFile.close();
    }
    else
    {
        qWarning() << "FileParseGuard: Не удалось создать файл-метку:" << _lockFilePath;
    }
}

void FileParseGuard::removeLockFile()
{
    if(!_lockFilePath.isEmpty())
    {
        if(QFile::exists(_lockFilePath))
        {
            if(QFile::remove(_lockFilePath))
            {
                // Успешно удалено
            }
            else
            {
                qWarning() << "FileParseGuard: Не удалось удалить файл-метку:" << _lockFilePath;
            }
        }
        _lockFilePath.clear();
    }
}


// FileLockRegistry (Singleton)
FileLockRegistry& FileLockRegistry::instance()
{
    static FileLockRegistry inst;
    return inst;
}

bool FileLockRegistry::acquire(const QString &filePath)
{
    QMutexLocker locker(&_mutex);

    if(_lockedFiles.contains(filePath))
    {
        return false;  // Уже заблокировано
    }

    _lockedFiles[filePath] = QThread::currentThreadId();
    return true;
}

void FileLockRegistry::release(const QString &filePath)
{
    QMutexLocker locker(&_mutex);
    _lockedFiles.remove(filePath);
}

bool FileLockRegistry::isLocked(const QString &filePath)
{
    QMutexLocker locker(&_mutex);
    return _lockedFiles.contains(filePath);
}
