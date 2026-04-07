#ifndef FC_PARSE_GUARDER_H
#define FC_PARSE_GUARDER_H

#include <QString>
#include <QFile>
#include <QMutex>
#include <QMap>
#include <QThread>

/**
 * @brief Класс для безопасной блокировки файла на время парсинга
 *
 * Использует комбинацию:
 * - flock() для блокировки доступа к файлу
 * - chattr +i для защиты от удаления (требуется root)
 * - Глобальный реестр для потокобезопасности
 *
 * @example
 * void parseFile(const QString &path) {
 *     FileParseGuard guard(path, true);  // true = использовать chattr
 *     if (guard.isLocked()) {
 *         // Работа с файлом...
 *     }
 * }  // guard автоматически снимет блокировку
 */
class FileParseGuard
{
public:
    /**
     * @brief Конструктор блокировки
     * @param filePath Путь к файлу для блокировки
     * @param useImmutable Если true и запущено от root — устанавливает chattr +i
     */
    explicit FileParseGuard(const QString &filePath, bool useImmutable = false);

    /**
     * @brief Деструктор — автоматически снимает блокировку
     */
    ~FileParseGuard();

    // Запрет копирования и перемещения
    FileParseGuard(const FileParseGuard&) = delete;
    FileParseGuard& operator=(const FileParseGuard&) = delete;
    FileParseGuard(FileParseGuard&&) = delete;
    FileParseGuard& operator=(FileParseGuard&&) = delete;

    /**
     * @brief Проверка, успешно ли заблокирован файл
     * @return true если блокировка активна
     */
    bool isLocked() const { return _locked; }

    /**
     * @brief Проверка, установлен ли флаг immutable
     * @return true если chattr +i установлен
     */
    bool isProtected() const { return _immutableSet; }

    /**
     * @brief Получить путь к заблокированному файлу
     * @return Путь к файлу
     */
    QString filePath() const { return _filePath; }

private:
    /**
     * @brief Установить блокировку
     */
    void lock();

    /**
     * @brief Снять блокировку
     */
    void unlock();

    /**
     * @brief Создать файл-метку с PID процесса
     */
    void createLockFile();

    /**
     * @brief Удалить файл-метку
     */
    void removeLockFile();

    // Данные
    QString _filePath;           ///< Путь к файлу
    QString _lockFilePath;       ///< Путь к файлу-метке PID
    bool _useImmutable;          ///< Флаг использования chattr +i
    bool _immutableSet;          ///< Флаг установленного chattr +i
    bool _registered;            ///< Флаг регистрации в реестре
    bool _locked;                ///< Флаг успешной блокировки flock
    int _fd;                     ///< Файловый дескриптор для flock
};


/**
 * @brief Глобальный реестр блокировок для потокобезопасности
 *
 * Предотвращает одновременную блокировку одного файла из разных потоков
 */
class FileLockRegistry
{
public:
    /**
     * @brief Получить единственный экземпляр реестра (Singleton)
     */
    static FileLockRegistry& instance();

    /**
     * @brief Попытка зарегистрировать блокировку файла
     * @param filePath Путь к файлу
     * @return true если успешно зарегистрировано
     */
    bool acquire(const QString &filePath);

    /**
     * @brief Снять регистрацию блокировки
     * @param filePath Путь к файлу
     */
    void release(const QString &filePath);

    /**
     * @brief Проверить, заблокирован ли файл
     * @param filePath Путь к файлу
     * @return true если файл уже заблокирован
     */
    bool isLocked(const QString &filePath);

private:
    FileLockRegistry() = default;
    ~FileLockRegistry() = default;

    // Запрет копирования
    FileLockRegistry(const FileLockRegistry&) = delete;
    FileLockRegistry& operator=(const FileLockRegistry&) = delete;

    QMutex _mutex;                              ///< Мьютекс для защиты реестра
    QMap<QString, Qt::HANDLE> _lockedFiles;     ///< Карта заблокированных файлов
};

#endif // FC_PARSE_GUARDER_H

// ПРИМЕР ИСПОЛЬЗОВАНИЯ
// main.cpp
//#include <QCoreApplication>
//#include "FCFileGuarder.h"
//#include <QFile>
//#include <QDebug>

//void parseFile(const QString &filePath) {
//    // Блокировка на время жизни объекта guard
//    FileParseGuard guard(filePath, true);  // true = использовать chattr +i

//    if (!guard.isLocked()) {
//        qWarning() << "Не удалось заблокировать файл!";
//        return;
//    }

//    if (guard.isProtected()) {
//        qDebug() << "Файл защищён от удаления (immutable)";
//    }

//    // Работа с файлом
//    QFile file(filePath);
//    if (file.open(QIODevice::ReadOnly)) {
//        QByteArray data = file.readAll();
//        qDebug() << "Прочитано байт:" << data.size();
//        // Парсинг данных...
//        file.close();
//    }

//    // guard автоматически снимет блокировку при выходе из функции
//}

//int main(int argc, char *argv[]) {
//    QCoreApplication app(argc, argv);

//    parseFile("/home/shared/data.txt");

//    return 0;
//}


// Пример использования в многопоточном режиме
// worker.cpp
//#include <QtConcurrent>
//#include "FCFileGuarder.h"

//void workerThread(const QString &filePath) {
//    FileParseGuard guard(filePath, true);

//    if (guard.isLocked()) {
//        // Парсинг...
//        qDebug() << "Поток" << QThread::currentThreadId()
//                 << "работает с" << filePath;
//    }
//}

//// Запуск
//QtConcurrent::run(workerThread, "/home/shared/file1.txt");
//QtConcurrent::run(workerThread, "/home/shared/file2.txt");
//QtConcurrent::run(workerThread, "/home/shared/file1.txt");  // Будет ждать


// Скрипт аварийной очистки
//#!/bin/bash
//# /usr/local/bin/cleanup_fcfileguarder.sh

//LOG_FILE="/var/log/fcfileguarder_cleanup.log"
//LOCK_DIR="/var/run/fcfileguarder"

//echo "=== $(date) ===" >> "$LOG_FILE"

//if [ ! -d "$LOCK_DIR" ]; then
//    echo "Директория не существует" >> "$LOG_FILE"
//    exit 0
//fi

//# Проверяем файлы с immutable флагом
// find /home/shared -type f -exec lsattr {} \; 2>/dev/null | \
//    grep -e 'i' | awk '{print $2}' | while read file; do

//    # Находим соответствующий lock-файл
//    hash=$(echo -n "$file" | md5sum | cut -d' ' -f1)
//    lock_file="$LOCK_DIR/${hash}.lock"

//    if [ -f "$lock_file" ]; then
//        pid=$(cat "$lock_file" 2>/dev/null)

//        if [ -n "$pid" ] && ! kill -0 "$pid" 2>/dev/null; then
//            echo "Процесс $pid мёртв, снимаем immutable: $file" >> "$LOG_FILE"
//            chattr -i "$file" 2>> "$LOG_FILE"
//            rm -f "$lock_file"
//        fi
//    else
//        echo "Нет метки, снимаем immutable: $file" >> "$LOG_FILE"
//        chattr -i "$file" 2>> "$LOG_FILE"
//    fi
//done

//echo "Очистка завершена" >> "$LOG_FILE"
