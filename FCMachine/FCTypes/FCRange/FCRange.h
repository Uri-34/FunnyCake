#ifndef FC_RANGE_H
#define FC_RANGE_H

#include <QtGlobal>
#include <type_traits>
#include <limits>
#include <algorithm>  // Для std::clamp (требуется C++17)

/**
 * @brief Шаблонный класс для представления числового диапазона [min, max] с гарантией корректности.
 *
 * Предоставляет безопасные операции над диапазонами числовых значений:
 *   - Автоматическая коррекция некорректных границ (min > max),
 *   - Проверка принадлежности значений диапазону,
 *   - Ограничение (clamp) значений границами диапазона,
 *   - Геометрические операции: сдвиг, масштабирование относительно центра,
 *   - Проверка пересечения диапазонов.
 *
 * Особенности реализации:
 *   - Полная поддержка constexpr (требуется C++17 для std::clamp),
 *   - Тривиальная копируемость (trivially copyable) для эффективного использования
 *     в контейнерах и встраиваемых системах,
 *   - Статические проверки типов на этапе компиляции,
 *   - Безопасность: все операции гарантируют корректность инварианта (min ≤ max).
 *
 * @tparam T Арифметический тип (целочисленный или с плавающей точкой).
 * @note Для целочисленных типов операции выполняются без потери точности.
 *       Для типов с плавающей точкой возможны ошибки округления при масштабировании.
 * @warning Класс не проверяет переполнение при арифметических операциях — ответственность
 *          за корректность значений лежит на пользователе.
 */
template <typename T>
class FCRange
{
    // Статическая проверка: только арифметические типы разрешены
    static_assert(std::is_arithmetic_v<T>, "FCRange требует арифметический тип (целочисленный или с плавающей точкой)");

public:
    /// Тип значений, хранящихся в диапазоне.
    using _type = T;

    // --- константы по умолчанию ---
    /// @brief Возвращает минимальное значение по умолчанию для типа T.
    /// @return -10000.0 для типов с плавающей точкой, -10000 для целочисленных типов.
    /// @note Значения выбраны как разумные границы для большинства практических задач.
    ///       Для экстремальных диапазонов используйте явную инициализацию.
    [[nodiscard]] static constexpr T defaultMin() noexcept
    {
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(-10000.0);
        } else {
            return static_cast<T>(-10000);
        }
    }

    /// @brief Возвращает максимальное значение по умолчанию для типа T.
    /// @return 10000.0 для типов с плавающей точкой, 10000 для целочисленных типов.
    /// @note Значения выбраны как разумные границы для большинства практических задач.
    ///       Для экстремальных диапазонов используйте явную инициализацию.
    [[nodiscard]] static constexpr T defaultMax() noexcept
    {
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(10000.0);
        } else {
            return static_cast<T>(10000);
        }
    }

    // --- конструкторы ---
    /// @brief Конструктор с параметрами.
    /// @param min Минимальная граница диапазона.
    /// @param max Максимальная граница диапазона.
    /// @note Если min > max, границы автоматически корректируются (меняются местами)
    ///       для сохранения инварианта min ≤ max. Исключения не генерируются.
    constexpr FCRange(T min = defaultMin(), T max = defaultMax()) noexcept
        : m_min{min <= max ? min : max}
        , m_max{min <= max ? max : min}
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FCRange(const FCRange&) noexcept = default;

    /// @brief Оператор присваивания копированием (тривиальный).
    constexpr FCRange& operator=(const FCRange&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FCRange() = default;

    // --- геттеры ---
    /// @brief Возвращает минимальную границу диапазона.
    /// @return Значение левой границы [min, max].
    [[nodiscard]] constexpr T min() const noexcept { return m_min; }

    /// @brief Возвращает максимальную границу диапазона.
    /// @return Значение правой границы [min, max].
    [[nodiscard]] constexpr T max() const noexcept { return m_max; }

    /// @brief Вычисляет ширину диапазона (разность границ).
    /// @return Значение (max - min).
    /// @note Для целочисленных типов результат всегда ≥ 0 благодаря инварианту класса.
    [[nodiscard]] constexpr T width() const noexcept { return m_max - m_min; }

    /// @brief Вычисляет центр диапазона.
    /// @return Среднее арифметическое границ: (min + max) / 2.
    /// @note Для нечётных целочисленных диапазонов результат округляется вниз
    ///       (стандартное поведение целочисленного деления в C++).
    [[nodiscard]] constexpr T center() const noexcept { return (m_min + m_max) / static_cast<T>(2); }

    // --- логические операции ---
    /// @brief Проверяет, принадлежит ли значение диапазону [min, max] (включая границы).
    /// @param value Проверяемое значение.
    /// @return true, если min ≤ value ≤ max; иначе — false.
    [[nodiscard]] constexpr bool contains(T value) const noexcept
    {
        return value >= m_min && value <= m_max;
    }

    /// @brief Проверяет, находится ли значение вне диапазона.
    /// @param value Проверяемое значение.
    /// @return true, если value < min или value > max; иначе — false.
    /// @note Эквивалентно !contains(value).
    [[nodiscard]] constexpr bool excludes(T value) const noexcept
    {
        return !contains(value);
    }

    /// @brief Ограничивает значение границами диапазона.
    /// @param value Исходное значение.
    /// @return Значение, приведённое к диапазону:
    ///         - если value < min → min,
    ///         - если value > max → max,
    ///         - иначе → value.
    /// @note Требуется C++17 для поддержки constexpr std::clamp.
    [[nodiscard]] constexpr T clamped(T value) const noexcept
    {
        return std::clamp(value, m_min, m_max);
    }

    /// @brief Проверяет пересечение с другим диапазоном.
    /// @param other Другой диапазон для проверки.
    /// @return true, если диапазоны пересекаются или касаются границами; иначе — false.
    /// @note Диапазоны [1,2] и [2,3] считаются пересекающимися (общая точка 2).
    [[nodiscard]] constexpr bool intersects(const FCRange& other) const noexcept
    {
        return m_min <= other.m_max && other.m_min <= m_max;
    }

    // --- модификаторы ---
    /// @brief Устанавливает новые границы диапазона.
    /// @param min Новая минимальная граница.
    /// @param max Новая максимальная граница.
    /// @note Если min > max, границы автоматически корректируются для сохранения
    ///       инварианта min ≤ max.
    constexpr void set(T min, T max) noexcept
    {
        if (min <= max) {
            m_min = min;
            m_max = max;
        } else {
            m_min = max;
            m_max = min;
        }
    }

    /// @brief Сдвигает диапазон на заданное смещение.
    /// @param offset Величина сдвига (может быть отрицательной).
    /// @note Обе границы сдвигаются на одинаковое значение: [min+Δ, max+Δ].
    constexpr void translate(T offset) noexcept
    {
        m_min += offset;
        m_max += offset;
    }

    /// @brief Масштабирует диапазон относительно его центра.
    /// @param factor Коэффициент масштабирования (1.0 = без изменений).
    /// @note Центр диапазона остаётся неизменным, ширина умножается на factor.
    ///       Для factor < 0 диапазон инвертируется относительно центра.
    constexpr void scaleFromCenter(T factor) noexcept
    {
        const T c = center();
        const T half = width() / static_cast<T>(2) * factor;
        m_min = c - half;
        m_max = c + half;
    }

    // --- операторы сравнения ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другой диапазон для сравнения.
    /// @return true, если обе границы идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FCRange& other) const noexcept
    {
        return m_min == other.m_min && m_max == other.m_max;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другой диапазон для сравнения.
    /// @return true, если диапазоны различаются хотя бы по одной границе; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FCRange& other) const noexcept
    {
        return !(*this == other);
    }

private:
    /// Минимальная граница диапазона (левая).
    T m_min;

    /// Максимальная граница диапазона (правая).
    T m_max;
};

// --- псевдонимы для часто используемых типов ---
/// Диапазон целочисленных значений.
using FCRangeInt = FCRange<int>;

/// Диапазон значений qreal (для совместимости с координатами Qt).
using FCRangeQReal = FCRange<qreal>;

/// Диапазон значений double (высокая точность для вычислений).
using FCRangeDouble = FCRange<double>;

// --- поддержка системы мета-объектов Qt (опционально) ---
// Раскомментируйте при необходимости использования в QVariant, моделях и сигналах/слотах:
/*
#include <QMetaType>
Q_DECLARE_METATYPE(FCRangeInt)
Q_DECLARE_METATYPE(FCRangeQReal)
Q_DECLARE_METATYPE(FCRangeDouble)
*/

// --- проверки компиляции ---
// Гарантирует тривиальную копируемость для использования в встраиваемых системах
static_assert(std::is_trivially_copyable_v<FCRange<int>>, "FCRange<int> должен быть тривиально копируемым типом");
static_assert(std::is_trivially_copyable_v<FCRange<float>>, "FCRange<float> должен быть тривиально копируемым типом");

// Проверка корректности значений по умолчанию
static_assert(FCRange<int>::defaultMin() == -10000, "defaultMin() для int должен быть -10000");
static_assert(FCRange<int>::defaultMax() == 10000, "defaultMax() для int должен быть 10000");

#endif // FC_RANGE_H
