#ifndef FC_I2C_BUS_H
#define FC_I2C_BUS_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QRecursiveMutex>
#include <cstdint>

#include "FCState.h"

/**
 * @brief Класс управления аппаратной шиной I²C на уровне операционной системы.
 *
 * Предоставляет потокобезопасный интерфейс для работы с шиной I²C:
 *   - Открытие/закрытие шины (например, /dev/i2c-1),
 *   - Операции чтения/записи с автоматической установкой состояния при ошибках,
 *   - Сканирование шины для обнаружения подключенных устройств,
 *   - Интеграция в систему управления состоянием через FCConditionObject.
 *
 * Особенности архитектуры:
 *   - ВСЕ методы ввода-вывода автоматически устанавливают состояние шины при ошибках
 *     через ГРУППОВЫЕ вызовы set(FCCondition{...}) для минимизации эмиссии сигналов,
 *   - Потокобезопасность обеспечивается через QRecursiveMutex (разрешает рекурсивные блокировки),
 *   - Метод scan() НЕ устанавливает состояние шины при ошибках опроса отдельных адресов —
 *     устанавливает ошибку ТОЛЬКО если шина не открыта (соответствует поведению i2cdetect),
 *   - При уничтожении объекта файл шины закрывается БЕЗ установки состояния (объект уничтожается),
 *   - Все операции с адресом устройства выполняются через ioctl(I2C_SLAVE) перед каждой транзакцией.
 *
 * Семантика ошибок шины:
 *   | Условие ошибки                  | Устанавливаемое состояние                         |
 *   |---------------------------------|---------------------------------------------------|
 *   | Ошибка открытия файла шины      | {Error, Stop, Changed, ConnectionError}           |
 *   | Ошибка ioctl (установка адреса) | {Error, Stop, Changed, ConnectionError}           |
 *   | Ошибка записи в шину            | {Error, Stop, Changed, WriteDataError}            |
 *   | Ошибка чтения из шины           | {Error, Stop, Changed, ReadDataError}             |
 *   | Успешная операция               | {Ready, Stop, NotChanged, NoError}                |
 *
 * @warning Критически важно:
 *          - Шина I²C должна существовать в файловой системе (например, /dev/i2c-1),
 *          - Для работы с шиной требуются права на чтение/запись (обычно группа i2c),
 *          - Все операции блокируют вызывающий поток на время транзакции,
 *          - Метод scan() может занять до 200 мс (зависит от параметра timeOutMs),
 *          - НИКОГДА не вызывайте методы ввода-вывода из конструктора базового класса.
 * @note Для встраиваемых систем (Raspberry Pi 4 + Buildroot):
 *       - Используйте ТОЛЬКО аппаратную шину I²C (не программную через GPIO),
 *       - Настройте права доступа к /dev/i2c-* через udev rules (группа i2c),
 *       - Избегайте частого сканирования шины в рабочем режиме — используйте только при инициализации,
 *       - Для критичных к времени операций выделяйте отдельный поток с приоритетом реального времени.
 * @see FCConditionObject для управления составным состоянием
 * @see FCI2CDevice для работы с конкретными устройствами на шине
 * @see FCLM75AThermometer для примера использования шины в драйвере устройства
 */

// --- псевдонимы типов ---
/// Список адресов устройств, обнаруженных на шине I²C (7-битные адреса 0x03–0x77).
using FCI2CDeviceAddressList = QList<uint8_t>;

class FCI2CBus
    : public QObject
{
Q_OBJECT

public:
//    inline static const FCBusState FCBusDefaultState {FCReadyState::NotReady, FCErrorType::None};

    // --- конструкторы и деструктор ---
    /// @brief Конструктор шины I²C с автоматическим открытием.
    /// @param path Путь к устройству шины в файловой системе (например, "/dev/i2c-1").
    /// @param parent Родительский QObject для управления временем жизни.
    /// @note При создании автоматически:
    ///       - Вызывается метод open() для открытия файла шины,
    ///       - При успешном открытии устанавливается состояние: {Ready, Stop, NotChanged, NoError},
    ///       - При ошибке открытия устанавливается: {Error, Stop, Changed, ConnectionError}.
    /// @warning Если файл шины не существует или нет прав доступа, состояние будет установлено в ошибку.
    explicit FCI2CBus(const QString &path, QObject *parent = nullptr);

    /// @brief Деструктор шины.
    /// @note Автоматически закрывает файл шины БЕЗ установки состояния (объект уничтожается).
    ///       Для ручного закрытия с установкой состояния используйте метод close().
    ~FCI2CBus() override;

    // --- запрет копирования ---
    /// @brief Запрет копирования объекта (удалён конструктор копирования).
    FCI2CBus(const FCI2CBus&) = delete;

    /// @brief Запрет присваивания объекта (удалён оператор присваивания).
    FCI2CBus& operator=(const FCI2CBus&) = delete;

    // --- методы управления шиной ---
    /// @brief Открывает доступ к шине I²C.
    /// @return true при успешном открытии; иначе false (состояние ошибки установлено автоматически).
    /// @note При ошибке устанавливается: {Error, Stop, Changed, ConnectionError}.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    /// @warning Если файл уже открыт, предварительно закрывает его и открывает заново.
    bool open();

    /// @brief Закрывает доступ к шине I²C.
    /// @note Устанавливает состояние: {NotReady, Stop, Changed, NoError}.
    ///       Для закрытия БЕЗ установки состояния (только в деструкторе) используйте прямой вызов _file.close().
    void close();

    /// @brief Проверяет, открыта ли шина для операций.
    /// @return true если файл шины открыт; иначе false.
    inline bool isOpen() { return _file.isOpen(); }

    /// @brief Возвращает путь к устройству шины в файловой системе.
    /// @return Строка вида "/dev/i2c-1".
    [[nodiscard]] QString path() const noexcept;

    // --- методы ввода-вывода с автоматической установкой состояния ---
    /// @brief Читает указанное количество байт из устройства на шине.
    /// @param address 7-битный адрес устройства (0x00–0x7F).
    /// @param count Количество байт для чтения.
    /// @param flags Флаги операции (зарезервировано для будущего использования, обычно 0).
    /// @return QByteArray с прочитанными данными, или пустой массив при ошибке.
    /// @note При ошибке устанавливается: {Error, Stop, Changed, ReadDataError}.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    /// @warning Блокирует вызывающий поток на время операции ввода-вывода.
    [[nodiscard]] QByteArray readBytes(uint8_t address, int count, int flags = 0);

    /// @brief Записывает массив байт в устройство на шине.
    /// @param address 7-битный адрес устройства (0x00–0x7F).
    /// @param data Данные для записи.
    /// @param flags Флаги операции (зарезервировано для будущего использования, обычно 0).
    /// @return true при успешной записи; иначе false.
    /// @note При ошибке устанавливается: {Error, Stop, Changed, WriteDataError}.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    /// @warning Блокирует вызывающий поток на время операции ввода-вывода.
    [[nodiscard]] bool writeBytes(uint8_t address, const QByteArray &data, int flags = 0);

    /// @brief Выполняет транзакцию write-then-read (для регистров с автоинкрементом).
    /// @param address 7-битный адрес устройства (0x00–0x7F).
    /// @param writeData Данные для записи (обычно адрес регистра).
    /// @param count Количество байт для чтения после записи.
    /// @param flags Флаги операции (зарезервировано для будущего использования, обычно 0).
    /// @return QByteArray с прочитанными данными, или пустой массив при ошибке.
    /// @note При ошибке устанавливается: {Error, Stop, Changed, ReadDataError} или WriteDataError.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    /// @warning Блокирует вызывающий поток на время операции ввода-вывода.
    [[nodiscard]] QByteArray writeRead(uint8_t address, const QByteArray &writeData, int count, int flags = 0);

    /// @brief Читает один байт из устройства на шине.
    /// @param address 7-битный адрес устройства (0x00–0x7F).
    /// @return Прочитанный байт, или 0 при ошибке.
    /// @note При ошибке устанавливается: {Error, Stop, Changed, ReadDataError}.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    [[nodiscard]] uint8_t readByte(uint8_t address);

    /// @brief Записывает один байт в устройство на шине.
    /// @param address 7-битный адрес устройства (0x00–0x7F).
    /// @param byte Значение для записи.
    /// @return true при успешной записи; иначе false.
    /// @note При ошибке устанавливается: {Error, Stop, Changed, WriteDataError}.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    [[nodiscard]] bool writeByte(uint8_t address, uint8_t byte);

    /// @brief Читает значение регистра устройства.
    /// @param address 7-битный адрес устройства (0x00–0x7F).
    /// @param reg Адрес регистра для чтения.
    /// @return Значение регистра, или 0 при ошибке.
    /// @note При ошибке устанавливается: {Error, Stop, Changed, ReadDataError}.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    [[nodiscard]] uint8_t readRegister(uint8_t address, uint8_t reg);

    /// @brief Записывает значение в регистр устройства.
    /// @param address 7-битный адрес устройства (0x00–0x7F).
    /// @param reg Адрес регистра для записи.
    /// @param value Значение для записи.
    /// @return true при успешной записи; иначе false.
    /// @note При ошибке устанавливается: {Error, Stop, Changed, WriteDataError}.
    ///       При успехе: {Ready, Stop, NotChanged, NoError}.
    [[nodiscard]] bool writeRegister(uint8_t address, uint8_t reg, uint8_t value);

    // --- методы диагностики ---
    /// @brief Сканирует шину и возвращает список обнаруженных устройств.
    /// @param timeOutMs Таймаут сканирования в миллисекундах (по умолчанию 200 мс).
    /// @return Список 7-битных адресов обнаруженных устройств (0x03–0x77).
    /// @note Метод НЕ устанавливает состояние шины при ошибках опроса отдельных адресов.
    ///       Устанавливает ошибку ТОЛЬКО если шина не открыта: {Error, Stop, Changed, ConnectionError}.
    ///       Соответствует поведению утилиты i2cdetect в Linux.
    /// @warning Может занять до 200 мс (зависит от timeOutMs и количества адресов).
    ///          Не рекомендуется вызывать в рабочем режиме — только при инициализации.
    [[nodiscard]] FCI2CDeviceAddressList scan(int timeOutMs = 200);

private:
    /// Файл устройства шины I²C (например, /dev/i2c-1).
    /// Владение: класс FCI2CBus.
    QFile _file;

    /// Рекурсивный мьютекс для потокобезопасного доступа к шине.
    /// Разрешает рекурсивные блокировки (полезно при вызове методов из методов класса).
    mutable QRecursiveMutex _mutex;
};

/// Список указателей на шины I²C для управления несколькими шинами.
using FCI2CBusList = QList<FCI2CBus*>;

/// Список слабых указателей на шины I²C для безопасного наблюдения.
using FCI2CBusQList = QList<QPointer<FCI2CBus>>;

// --- проверки компиляции ---
// Гарантирует, что адрес устройства умещается в 7 бит
static_assert(sizeof(uint8_t) == 1, "uint8_t должен быть 1 байтом");

#endif // FC_I2C_BUS_H
