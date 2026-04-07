#ifndef FC_COMMAND_H
#define FC_COMMAND_H

#include <QString>
#include <QChar>
#include <QMap>
#include <QVariant>
#include <QMetaType>
#include <QRegularExpression>
#include <QDebug>
#include <optional>
#include <cmath>

/**
 * @brief Класс для хранения, парсинга и генерации G-code команд (совместимость с прошивкой Marlin 2.1.2.4).
 *
 * Предназначен для работы с числовыми управляющими командами 3D-принтеров и ЧПУ-станков:
 *   - G-коды: геометрические движения и режимы (G0-G999),
 *   - M-коды: вспомогательные функции и управление (M0-M999),
 *   - T-коды: выбор активного экструдера (T0-T999).
 *
 * Особенности реализации:
 *   - Полная интеграция с фреймворком Qt (QString, QMap, Q_GADGET),
 *   - Поддержка интроспекции через систему мета-объектов Qt (Q_PROPERTY, Q_ENUM),
 *   - Автоматическая валидация синтаксиса и контрольных сумм,
 *   - Умная сериализация с контролем формата чисел с плавающей точкой,
 *   - Готовность к использованию в QVariant, моделях данных и сетевых протоколах.
 *
 * @note Класс нетривиален из-за использования QString и QMap, что исключает применение
 *       в строго ограниченных средах (например, прерываниях микроконтроллеров). Для встраиваемых
 *       систем рекомендуется использовать только в основной задаче обработки команд.
 */
class FCCommand
{
    Q_GADGET
    Q_PROPERTY(Type type READ type)
    Q_PROPERTY(int number READ number)
    Q_PROPERTY(int lineNumber READ lineNumber)
    Q_PROPERTY(int checksum READ checksum)
    Q_PROPERTY(QString comment READ comment WRITE setComment)
    Q_PROPERTY(QMap<QChar, float> params READ params)

public:
    /// Типы поддерживаемых G-code команд.
    enum Type
    {
        Unknown = 0,  ///< Неизвестная или некорректная команда.
        GCode   = 1,  ///< Геометрические команды и режимы (G0–G999).
        MCode   = 2,  ///< Вспомогательные функции и управление (M0–M999).
        TCode   = 3   ///< Выбор активного экструдера (T0–T999).
    };
    Q_ENUM(Type)

    /// @brief Конструктор по умолчанию. Создаёт невалидную команду типа Unknown.
    FCCommand() = default;

    /// @brief Конструктор с парсингом G-code строки.
    /// @param gcodeLine Строка G-code для парсинга (например, "G1 X10.5 Y20.0 F1500 ; move").
    /// @note Автоматически вызывает метод parse(). Результат валидности можно проверить через isValid().
    explicit FCCommand(const QString &gcodeLine);

    // --- парсинг и валидация ---
    /// @brief Парсит строку G-code и заполняет поля объекта.
    /// @param gcodeLine Строка G-code для анализа.
    /// @return true, если строка успешно распознана как валидная команда; иначе — false.
    /// @note Поддерживает:
    ///       - номера строк (префикс "N123 "),
    ///       - комментарии (после ";" или "()"),
    ///       - контрольные суммы (суффикс "*42"),
    ///       - параметры вида "X10.5 Y-20 Z3.14".
    /// @warning Метод сбрасывает все предыдущие значения полей перед парсингом.
    bool parse(const QString &gcodeLine);

    /// @brief Проверяет валидность команды.
    /// @return true, если команда распознана (тип ≠ Unknown и номер ≥ 0); иначе — false.
    /// @note Не гарантирует семантической корректности параметров (например, отрицательная подача).
    [[nodiscard]] bool isValid() const noexcept { return _type != Unknown && _number >= 0; }

    // --- тип и номер команды ---
    /// @brief Возвращает тип команды (GCode, MCode, TCode или Unknown).
    /// @return Значение перечисления Type.
    [[nodiscard]] Type type() const noexcept { return _type; }

    /// @brief Возвращает числовой номер команды.
    /// @return Номер команды (например, 1 для "G1", 109 для "M109"), либо -1 для невалидной команды.
    [[nodiscard]] int number() const noexcept { return _number; }

    /// @brief Возвращает букву-идентификатор типа команды.
    /// @return 'G' для GCode, 'M' для MCode, 'T' для TCode, '\0' для Unknown.
    [[nodiscard]] QChar typeChar() const noexcept;

    /// @brief Возвращает строковое представление типа и номера команды.
    /// @return Строка вида "G1", "M109", "T0" или пустая строка для невалидной команды.
    [[nodiscard]] QString typeString() const noexcept;

    // --- работа с параметрами ---
    /// @brief Проверяет наличие параметра с заданной буквой.
    /// @param letter Буква параметра (например, 'X', 'Y', 'F').
    /// @return true, если параметр присутствует в команде; иначе — false.
    [[nodiscard]] bool hasParam(QChar letter) const noexcept;

    /// @brief Возвращает значение параметра как число с плавающей точкой.
    /// @param letter Буква параметра (например, 'X', 'Y', 'F').
    /// @return Значение параметра, либо std::nullopt если параметр отсутствует.
    [[nodiscard]] std::optional<float> param(QChar letter) const noexcept;

    /// @brief Возвращает значение параметра как целое число (с округлением).
    /// @param letter Буква параметра.
    /// @return Округлённое до целого значение параметра, либо std::nullopt если параметр отсутствует.
    /// @note Использует математическое округление (round-half-away-from-zero).
    [[nodiscard]] std::optional<int> paramInt(QChar letter) const noexcept;

    /// @brief Возвращает значение параметра как логическое (для флагов).
    /// @param letter Буква параметра.
    /// @return true если значение ≠ 0, иначе false; std::nullopt если параметр отсутствует.
    [[nodiscard]] std::optional<bool> paramBool(QChar letter) const noexcept;

    /// @brief Устанавливает или обновляет значение параметра.
    /// @param letter Буква параметра (должна быть латинской заглавной буквой).
    /// @param value  Числовое значение параметра.
    /// @note Если параметр уже существует — значение заменяется. Параметры хранятся в QMap
    ///       для сохранения порядка при сериализации (лексикографический порядок букв).
    void setParam(QChar letter, float value);

    /// @brief Устанавливает значение параметра из целого числа.
    /// @param letter Буква параметра.
    /// @param value  Целочисленное значение.
    /// @note Автоматически преобразует в float перед сохранением.
    void setParam(QChar letter, int value) { setParam(letter, static_cast<float>(value)); }

    /// @brief Удаляет параметр с заданной буквой.
    /// @param letter Буква параметра для удаления.
    /// @note Если параметр отсутствует — операция игнорируется без ошибок.
    void removeParam(QChar letter);

    /// @brief Возвращает все параметры команды.
    /// @return QMap с парами (буква → значение). Параметры упорядочены по возрастанию буквы.
    [[nodiscard]] QMap<QChar, float> params() const noexcept { return _params; }

    // --- специальные поля (номер строки, контрольная сумма, комментарий) ---
    /// @brief Возвращает номер строки из префикса "Nxxx".
    /// @return Номер строки, либо -1 если номер не задан.
    /// @note Номер строки не влияет на семантику команды, используется для отладки и подтверждения получения.
    [[nodiscard]] int lineNumber() const noexcept { return _lineNumber.value_or(-1); }

    /// @brief Возвращает контрольную сумму из суффикса "*xx".
    /// @return Значение контрольной суммы (результат XOR всех байтов до '*'), либо -1 если отсутствует.
    [[nodiscard]] int checksum() const noexcept { return _checksum.value_or(-1); }

    /// @brief Проверяет наличие контрольной суммы в исходной строке.
    /// @return true, если команда содержала контрольную сумму; иначе — false.
    [[nodiscard]] bool hasChecksum() const noexcept { return _checksum.has_value(); }

    /// @brief Возвращает комментарий к команде (после ";" или внутри "()").
    /// @return Текст комментария без разделителей, либо пустая строка если комментарий отсутствует.
    [[nodiscard]] QString comment() const noexcept { return _comment; }

    /// @brief Устанавливает комментарий к команде.
    /// @param comment Текст комментария (разделители ";" добавляются автоматически при сериализации).
    /// @note Перед сохранением выполняется обрезка пробельных символов по краям (trimmed()).
    void setComment(const QString &comment) { _comment = comment.trimmed(); }

    // --- сериализация в строку ---
    /// @brief Преобразует команду в строку G-code.
    /// @param withChecksum    Добавлять ли контрольную сумму (суффикс "*xx").
    /// @param withLineNumber  Добавлять ли номер строки (префикс "Nxxx ").
    /// @return Строка в формате G-code, готовая к отправке принтеру.
    /// @note Форматирование чисел с плавающей точкой оптимизировано: целые значения выводятся без дробной части,
    ///       дробные — с минимальным необходимым количеством знаков после запятой.
    [[nodiscard]] QString toString(bool withChecksum = false, bool withLineNumber = false) const;

    // --- утилиты для часто используемых параметров ---
    /// @brief Возвращает координату X (для команд перемещения).
    /// @return Значение параметра X, либо std::nullopt если отсутствует.
    [[nodiscard]] std::optional<float> x() const noexcept { return param(QLatin1Char('X')); }

    /// @brief Возвращает координату Y (для команд перемещения).
    /// @return Значение параметра Y, либо std::nullopt если отсутствует.
    [[nodiscard]] std::optional<float> y() const noexcept { return param(QLatin1Char('Y')); }

    /// @brief Возвращает координату Z (для команд перемещения).
    /// @return Значение параметра Z, либо std::nullopt если отсутствует.
    [[nodiscard]] std::optional<float> z() const noexcept { return param(QLatin1Char('Z')); }

    /// @brief Возвращает позицию экструдера E (количество поданного филамента).
    /// @return Значение параметра E, либо std::nullopt если отсутствует.
    [[nodiscard]] std::optional<float> e() const noexcept { return param(QLatin1Char('E')); }

    /// @brief Возвращает скорость подачи (feedrate) в мм/мин.
    /// @return Значение параметра F, либо std::nullopt если отсутствует.
    [[nodiscard]] std::optional<float> f() const noexcept { return param(QLatin1Char('F')); }

    /// @brief Возвращает скорость вращения шпинделя или мощность лазера (S).
    /// @return Значение параметра S (обычно 0–255 для ШИМ или об/мин для шпинделя), либо std::nullopt.
    [[nodiscard]] std::optional<float> s() const noexcept { return param(QLatin1Char('S')); }

    // --- интеграция с системой типов Qt ---
    /// @brief Преобразует команду в QVariant для использования в моделях и сигналах/слотах.
    /// @return QVariant типа FCCommand.
    /// @note Требует предварительной регистрации типа через qRegisterMetaType<FCCommand>().
    [[nodiscard]] QVariant toVariant() const;

    /// @brief Восстанавливает команду из QVariant.
    /// @param variant QVariant, содержащий FCCommand.
    /// @return Восстановленный объект команды.
    /// @note Возвращает невалидную команду (Unknown), если variant имеет неверный тип.
    static FCCommand fromVariant(const QVariant &variant);

    // --- отладка ---
    /// @brief Оператор вывода в отладочный поток.
    /// @param dbg Отладочный поток (QDebug).
    /// @param cmd Команда для вывода.
    /// @return Ссылка на отладочный поток.
    /// @note Формат вывода: "G1 X10.5 Y20 F1500 ; comment" или "<invalid>" для невалидных команд.
    friend QDebug operator<<(QDebug dbg, const FCCommand &cmd);

private:
    /// Тип команды (GCode, MCode, TCode или Unknown).
    Type _type = Unknown;

    /// Числовой номер команды (например, 1 для "G1").
    int _number = -1;

    /// Номер строки из префикса "Nxxx" (опционально).
    std::optional<int> _lineNumber;

    /// Контрольная сумма из суффикса "*xx" (опционально).
    std::optional<int> _checksum;

    /// Параметры команды в формате "буква → значение" (например, 'X' → 10.5).
    /// QMap используется вместо QHash для сохранения лексикографического порядка при сериализации.
    QMap<QChar, float> _params;

    /// Комментарий к команде (без разделителей ";" или "()").
    QString _comment;

    // --- вспомогательные методы ---
    /// @brief Проверяет, является ли символ пробельным (пробел, табуляция, перевод строки).
    /// @param c Проверяемый символ.
    /// @return true, если символ является разделителем; иначе — false.
    [[nodiscard]] static bool isWhitespace(QChar c) noexcept;

    /// @brief Проверяет, является ли символ десятичной цифрой.
    /// @param c Проверяемый символ.
    /// @return true, если символ в диапазоне '0'–'9'; иначе — false.
    [[nodiscard]] static bool isDigit(QChar c) noexcept;

    /// @brief Проверяет, является ли символ латинской буквой.
    /// @param c Проверяемый символ.
    /// @return true, если символ в диапазоне 'A'–'Z' или 'a'–'z'; иначе — false.
    [[nodiscard]] static bool isLetter(QChar c) noexcept;

    /// @brief Проверяет корректность контрольной суммы в исходной строке.
    /// @param rawLine Исходная строка с суффиксом "*xx".
    /// @return true, если вычисленная контрольная сумма совпадает с указанной; иначе — false.
    [[nodiscard]] bool verifyChecksum(const QString &rawLine) const noexcept;

    /// @brief Вычисляет контрольную сумму для строки команды (алгоритм Marlin).
    /// @param cmd Строка команды без номера строки и контрольной суммы.
    /// @return Результат побитового XOR всех байтов строки.
    /// @note Алгоритм: для каждого байта результата применяется операция ^= (исключающее ИЛИ).
    [[nodiscard]] int computeChecksum(const QString &cmd) const noexcept;

    /// @brief Форматирует число с плавающей точкой для G-code.
    /// @param value Числовое значение.
    /// @return Строка без лишних нулей: целые как "10", дробные как "10.5" или "0.001".
    /// @note Оптимизировано для минимизации размера передаваемых данных по последовательному порту.
    [[nodiscard]] static QString formatFloat(float value);
};

// Регистрация типа для системы мета-объектов Qt
Q_DECLARE_METATYPE(FCCommand)

#endif // FC_COMMAND_H
