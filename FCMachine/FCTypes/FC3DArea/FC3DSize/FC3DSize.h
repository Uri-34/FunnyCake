#ifndef FC_3D_SIZE_H
#define FC_3D_SIZE_H

#include <QtGlobal>
#include <type_traits>
#include <cmath>

class FC3DPoint;

/**
 * @brief Класс для представления трёхмерного размера (ширина × высота × глубина).
 *
 * Семантически отличается от точки (FC3DPoint):
 *   - координата X интерпретируется как ширина (width),
 *   - координата Y интерпретируется как высота (height),
 *   - координата Z интерпретируется как глубина (depth).
 *
 * Не наследует FC3DPoint — это отдельная концептуальная сущность с собственной семантикой.
 * Класс спроектирован как тривиальный (trivially copyable) для эффективной работы
 * в контейнерах и возможности использования в высокопроизводительных сценариях,
 * включая встраиваемые системы.
 */
class FC3DSize
{
public:
    /// Тип данных, используемый для хранения размеров (по умолчанию — float).
    using _type = qreal;

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт размер с нулевыми значениями (0, 0, 0).
    constexpr FC3DSize() noexcept = default;

    /// @brief Конструктор с параметрами.
    /// @param w Ширина (значение по оси X).
    /// @param h Высота (значение по оси Y).
    /// @param d Глубина (значение по оси Z).
    constexpr FC3DSize(_type w, _type h, _type d) noexcept
        : _w(w)
        , _h(h)
        , _d(d)
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FC3DSize(const FC3DSize&) noexcept = default;

    /// @brief Оператор присваивания копированием (тривиальный).
    constexpr FC3DSize& operator=(const FC3DSize&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FC3DSize() = default;

    // --- геттеры ---
    /// @brief Возвращает ширину размера.
    /// @return Значение ширины (компонента по оси X).
    [[nodiscard]] constexpr _type width() const noexcept { return _w; }

    /// @brief Возвращает высоту размера.
    /// @return Значение высоты (компонента по оси Y).
    [[nodiscard]] constexpr _type height() const noexcept { return _h; }

    /// @brief Возвращает глубину размера.
    /// @return Значение глубины (компонента по оси Z).
    [[nodiscard]] constexpr _type depth() const noexcept { return _d; }

    // --- сеттеры ---
    /// @brief Устанавливает новое значение ширины.
    /// @param w Новое значение ширины.
    constexpr void setWidth(_type w) noexcept { _w = w; }

    /// @brief Устанавливает новое значение высоты.
    /// @param h Новое значение высоты.
    constexpr void setHeight(_type h) noexcept { _h = h; }

    /// @brief Устанавливает новое значение глубины.
    /// @param d Новое значение глубины.
    constexpr void setDepth(_type d) noexcept { _d = d; }

    /// @brief Устанавливает все три размера одновременно.
    /// @param w Новое значение ширины.
    /// @param h Новое значение высоты.
    /// @param d Новое значение глубины.
    constexpr void setWidthHeightDepth(_type w, _type h, _type d) noexcept
    {
        _w = w;
        _h = h;
        _d = d;
    }

    // --- преобразования ---
    /// @brief Преобразует размер в точку, интерпретируя компоненты как координаты.
    /// @return Точка с координатами (width, height, depth).
    /// @note Определение находится после объявления FC3DPoint (из-за циклической зависимости).
    [[nodiscard]] constexpr FC3DPoint toPoint() const noexcept;

    /// @brief Создаёт размер из точки, интерпретируя координаты как компоненты размера.
    /// @param p Исходная точка (координаты интерпретируются как width = x, height = y, depth = z).
    /// @return Новый размер на основе координат точки.
    [[nodiscard]] static constexpr FC3DSize fromPoint(const FC3DPoint& p) noexcept;

    // --- проверки состояния ---
    /// @brief Проверяет, является ли размер пустым (неположительное значение хотя бы по одной оси).
    /// @return true, если ширина ≤ 0 или высота ≤ 0 или глубина ≤ 0; иначе — false.
    /// @note Семантика соответствует QSize::isEmpty() в Qt: любая неположительная компонента делает размер пустым.
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return _w <= 0 || _h <= 0 || _d <= 0; }

    /// @brief Проверяет, является ли размер нулевым (все компоненты точно равны нулю).
    /// @return true, если ширина = 0 и высота = 0 и глубина = 0; иначе — false.
    [[nodiscard]] constexpr bool isNull() const noexcept { return _w == 0 && _h == 0 && _d == 0; }

    // --- геометрические свойства ---
    /// @brief Вычисляет объём параллелепипеда с заданным размером.
    /// @return Произведение ширины × высоты × глубины.
    /// @note Результат может быть отрицательным, если одна или три компоненты отрицательны.
    [[nodiscard]] constexpr _type volume() const noexcept { return _w * _h * _d; }

    /// @brief Вычисляет длину пространственной диагонали параллелепипеда.
    /// @return Евклидово расстояние от (0,0,0) до (width, height, depth).
    /// @note Требует C++23 для constexpr-контекста из-за std::hypot.
    [[nodiscard]] /*constexpr*/ _type diagonal() const noexcept
    {
        return std::hypot(_w, std::hypot(_h, _d));
    }

    // --- операторы сравнения ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другой размер для сравнения.
    /// @return true, если все три компоненты идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC3DSize& other) const noexcept
    {
        return _w == other._w && _h == other._h && _d == other._d;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другой размер для сравнения.
    /// @return true, если размеры различаются хотя бы по одной компоненте; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC3DSize& other) const noexcept
    {
        return !(*this == other);
    }

    // --- операторы модификации ---
    /// @brief Оператор составного сложения размеров (покомпонентное сложение).
    /// @param other Добавляемый размер.
    /// @return Ссылка на текущий объект после модификации.
    constexpr FC3DSize& operator+=(const FC3DSize& other) noexcept
    {
        _w += other._w;
        _h += other._h;
        _d += other._d;
        return *this;
    }

    /// @brief Оператор сложения размеров (покомпонентное сложение).
    /// @param other Добавляемый размер.
    /// @return Новый размер с суммой соответствующих компонентов.
    [[nodiscard]] constexpr FC3DSize operator+(const FC3DSize& other) const noexcept
    {
        return FC3DSize{_w + other._w, _h + other._h, _d + other._d};
    }

    // --- масштабирование ---
    /// @brief Возвращает размер, масштабированный единым коэффициентом.
    /// @param factor Коэффициент масштабирования для всех осей.
    /// @return Новый размер с умноженными на фактор компонентами.
    [[nodiscard]] constexpr FC3DSize scaled(_type factor) const noexcept
    {
        return FC3DSize{_w * factor, _h * factor, _d * factor};
    }

    /// @brief Возвращает размер, масштабированный разными коэффициентами по осям.
    /// @param fx Коэффициент масштабирования по оси X (ширина).
    /// @param fy Коэффициент масштабирования по оси Y (высота).
    /// @param fz Коэффициент масштабирования по оси Z (глубина).
    /// @return Новый размер с независимо масштабированными компонентами.
    [[nodiscard]] constexpr FC3DSize scaled(_type fx, _type fy, _type fz) const noexcept
    {
        return FC3DSize{_w * fx, _h * fy, _d * fz};
    }

private:
    /// Ширина размера (интерпретируется как компонент X).
    _type _w = 0;

    /// Высота размера (интерпретируется как компонент Y).
    _type _h = 0;

    /// Глубина размера (интерпретируется как компонент Z).
    _type _d = 0;
};

/// @brief Предопределённый "ошибочный" размер со значениями (-1, -1, -1).
/// Используется как признак ошибки или недопустимого состояния (аналог NaN для 3D-геометрии).
constexpr FC3DSize FC3DSizeError{-1, -1, -1};

// Проверки компиляции (гарантируют тривиальность и предсказуемый размер в памяти)
// static_assert(std::is_trivially_copyable_v<FC3DSize>, "FC3DSize должен быть тривиально копируемым типом");
// static_assert(sizeof(FC3DSize) == 3 * sizeof(float), "Размер FC3DSize должен быть в точности 3 × float");

#endif // FC_3D_SIZE_H
