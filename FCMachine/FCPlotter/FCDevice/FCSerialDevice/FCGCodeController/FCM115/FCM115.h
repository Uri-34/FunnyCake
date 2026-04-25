#ifndef FC_MARLIN_M115_H
#define FC_MARLIN_M115_H

#include <QString>
#include <QMetaType>

#include "FC3DArea.h"
#include "FCSpeed.h"

/**
 * @brief Контейнер данных диагностической информации прошивки Marlin (команда M115).
 *
 * Хранит ПАССИВНЫЕ данные, полученные из ответа контроллера:
 *   - Имя и версия прошивки,
 *   - Параметры машины (имя, рабочая область, скорости),
 *   - Идентификаторы совместимости (протокол, UUID).
 *
 * Особенности:
 *   - НЕ является устройством — НЕ имеет состояния жизненного цикла,
 *   - Парсинг выполняется ОДИН РАЗ при инициализации,
 *   - Все данные доступны через константные геттеры,
 *   - Проверка валидности через метод isValid().
 *
 * @note Для интеграции в сигналы/слоты зарегистрирован как метатип:
 *       @code
 *       Q_DECLARE_METATYPE(FCM115)
 *       @endcode
 * @see FCMarlinController для получения и парсинга данных из контроллера
 */
class FCM115
{
public:
    // --- константы ---
    /// @brief Ожидаемый UUID прошивки для проверки совместимости.
    static constexpr const char* EXPECTED_UUID = "50ca114a-fdc8-41e0-9912-bfe7ec3d20bb";

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию (пустые данные, невалидный объект).
    FCM115() = default;

    /// @brief Конструктор с парсингом ответа контроллера.
    /// @param response Полный текст ответа на команду M115.
    /// @note При ошибке парсинга объект остаётся в невалидном состоянии (isValid() == false).
    explicit FCM115(const QString &response) { parse(response); }

    // --- методы парсинга ---
    /// @brief Парсит ответ контроллера и заполняет поля объекта.
    /// @param response Текст ответа контроллера.
    /// @return true при успешном парсинге всех критических параметров; иначе false.
    bool parse(const QString &response);

    // --- проверка валидности ---
    /// @brief Проверяет, успешно ли выполнен парсинг всех критических параметров.
    /// @return true если все обязательные поля заполнены; иначе false.
    [[nodiscard]] bool isValid() const noexcept { return _isValid; }

    /// @brief Проверяет совместимость прошивки по UUID.
    /// @return true если UUID совпадает с ожидаемым; иначе false.
    [[nodiscard]] bool isCompatible() const noexcept { return _uuid == QString::fromUtf8(EXPECTED_UUID); }

    // --- методы доступа к данным ---
    /// @brief Возвращает имя прошивки.
    /// @return Строка с именем прошивки.
    [[nodiscard]] const QString& firmware() const noexcept { return _firmware; }

    /// @brief Возвращает версию прошивки.
    /// @return Строка с версией прошивки.
    [[nodiscard]] const QString& version() const noexcept { return _version; }

    /// @brief Возвращает имя/тип машины.
    /// @return Строка с идентификатором машины.
    [[nodiscard]] const QString& machine() const noexcept { return _machine; }

    /// @brief Возвращает параметры рабочей области машины.
    /// @return Константная ссылка на объект FC3DArea.
    [[nodiscard]] const FC3DArea& area() const noexcept { return _area; }

    /// @brief Возвращает настройки скоростей машины.
    /// @return Константная ссылка на объект FCSpeed.
    [[nodiscard]] const FCSpeed& speeds() const noexcept { return _speeds; }

    /// @brief Возвращает UUID прошивки.
    /// @return Строка с UUID.
    [[nodiscard]] const QString& uuid() const noexcept { return _uuid; }

private:
    /// @brief Парсит строку с типом машины и инициализирует поле _machine.
    /// @param machineTypeStr Строка с описанием типа машины из ответа контроллера.
    /// @return true при успешном парсинге; иначе false.
    [[nodiscard]] bool parseMachineType(const QString &machineTypeStr);

    /// @brief Извлекает значение поля по ключу из ответа контроллера.
    /// @param response Полный текст ответа на команду M115.
    /// @param key Имя искомого поля (например, "FIRMWARE_NAME", "UUID").
    /// @return Значение поля или пустая строка, если ключ не найден.
    [[nodiscard]] static QString extractField(const QString &response, const QString &key);

    // --- данные прошивки ---
    /// @brief Имя прошивки (например, "Marlin").
    QString _firmware;

    /// @brief Версия прошивки (например, "2.1.2").
    QString _version;

    /// @brief Идентификатор или название машины.
    QString _machine;

    /// @brief Параметры рабочей области (размеры по осям X, Y, Z).
    FC3DArea _area;

    /// @brief Настройки максимальных скоростей перемещения.
    FCSpeed _speeds;

    /// @brief Уникальный идентификатор прошивки для проверки совместимости.
    QString _uuid;

    // --- флаг валидности ---
    /// @brief Флаг, указывающий на успешность парсинга критических параметров.
    bool _isValid = false;
};

// Регистрация для использования в сигналах/слотах
Q_DECLARE_METATYPE(FCM115)

#endif // FC_MARLIN_M115_H
