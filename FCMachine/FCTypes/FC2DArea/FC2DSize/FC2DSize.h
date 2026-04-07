#ifndef FC_2D_SIZE_H
#define FC_2D_SIZE_H

#include <QtGlobal>
#include <QList>
#include <type_traits>

#include "FC2DPoint.h"

//class FC2DPoint;

/**
 * @brief Класс для представления двумерного размера (ширина × высота).
 *
 * Семантически отличается от точки (FC2DPoint):
 *   - координата X интерпретируется как ширина (width),
 *   - координата Y интерпретируется как высота (height).
 *
 * Не наследует FC2DPoint — это разные концептуальные сущности.
 * Для взаимных преобразований используйте toPoint() и fromPoint().
 * Класс спроектирован как тривиальный (trivially copyable) для эффективной работы
 * в контейнерах и возможности использования в constexpr-контекстах.
 */
class FC2DSize
{
public:
    /// Тип данных, используемый для хранения размеров (по умолчанию — float).
    using _type = float;

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт размер с нулевыми значениями (0, 0).
    constexpr FC2DSize() noexcept = default;

    /// @brief Конструктор с параметрами.
    /// @param w Ширина (значение по оси X).
    /// @param h Высота (значение по оси Y).
    constexpr FC2DSize(_type w, _type h) noexcept
        : _width(w),
          _height(h)
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FC2DSize(const FC2DSize&) noexcept = default;

    /// @brief Оператор присваивания копированием (тривиальный).
    constexpr FC2DSize& operator=(const FC2DSize&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FC2DSize() = default;

    // --- геттеры ---
    /// @brief Возвращает ширину размера.
    /// @return Значение ширины.
    [[nodiscard]] constexpr _type width() const noexcept { return _width; }

    /// @brief Возвращает высоту размера.
    /// @return Значение высоты.
    [[nodiscard]] constexpr _type height() const noexcept { return _height; }

    // --- сеттеры ---
    /// @brief Устанавливает новое значение ширины.
    /// @param w Новое значение ширины.
    constexpr void setWidth(_type w) noexcept { _width = w; }

    /// @brief Устанавливает новое значение высоты.
    /// @param h Новое значение высоты.
    constexpr void setHeight(_type h) noexcept { _height = h; }

    /// @brief Устанавливает оба размера одновременно.
    /// @param w Новое значение ширины.
    /// @param h Новое значение высоты.
    constexpr void setWidthHeight(_type w, _type h) noexcept
    {
        _width = w;
        _height = h;
    }

    // --- преобразования ---
    /// @brief Преобразует размер в точку, интерпретируя ширину как X, высоту как Y.
    /// @return Точка с координатами (width, height).
    /// @note Определение находится после объявления FC2DPoint (из-за циклической зависимости).
    [[nodiscard]] constexpr FC2DPoint toPoint() const noexcept { return FC2DPoint{_width, _height}; }

    /// @brief Создаёт размер из точки, интерпретируя координаты как ширину и высоту.
    /// @param p Исходная точка (координаты интерпретируются как width = x, height = y).
    /// @return Новый размер на основе координат точки.
    [[nodiscard]] static constexpr FC2DSize fromPoint(FC2DPoint &p) noexcept { return FC2DSize{p.x(), p.y()}; }

    // --- проверки состояния ---
    /// @brief Проверяет, является ли размер нулевым (точно 0×0).
    /// @return true, если ширина и высота равны нулю; иначе — false.
    [[nodiscard]] constexpr bool isNull() const noexcept { return _width == 0 && _height == 0; }

    /// @brief Проверяет, является ли размер пустым (неположительная ширина или высота).
    /// @return true, если ширина <= 0 или высота <= 0; иначе — false.
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return _width <= 0 || _height <= 0; }

    /// @brief Вычисляет площадь прямоугольника с заданным размером.
    /// @return Произведение ширины на высоту (ширина × высота).
    [[nodiscard]] constexpr _type square() const noexcept { return _width * _height; }

    // --- операторы ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другой размер для сравнения.
    /// @return true, если ширина и высота идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC2DSize& other) const noexcept
    {
        return _width == other._width && _height == other._height;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другой размер для сравнения.
    /// @return true, если размеры различаются по ширине или высоте; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC2DSize& other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Оператор составного сложения размеров (покомпонентное сложение).
    /// @param other Добавляемый размер.
    /// @return Ссылка на текущий объект после модификации.
    constexpr FC2DSize& operator+=(const FC2DSize& other) noexcept
    {
        _width += other._width;
        _height += other._height;
        return *this;
    }

    /// @brief Оператор сложения размеров (покомпонентное сложение).
    /// @param other Добавляемый размер.
    /// @return Новый размер с суммой соответствующих компонентов.
    [[nodiscard]] constexpr FC2DSize operator+(const FC2DSize& other) const noexcept
    {
        return FC2DSize{_width + other._width, _height + other._height};
    }

    // --- масштабирование ---
    /// @brief Возвращает размер, масштабированный единым коэффициентом.
    /// @param factor Коэффициент масштабирования для обеих осей.
    /// @return Новый размер с умноженными на фактор компонентами.
    [[nodiscard]] constexpr FC2DSize scaled(_type factor) const noexcept
    {
        return FC2DSize{_width * factor, _height * factor};
    }

    /// @brief Возвращает размер, масштабированный разными коэффициентами по осям.
    /// @param factorX Коэффициент масштабирования по оси X (ширина).
    /// @param factorY Коэффициент масштабирования по оси Y (высота).
    /// @return Новый размер с независимо масштабированными компонентами.
    [[nodiscard]] constexpr FC2DSize scaled(_type factorX, _type factorY) const noexcept
    {
        return FC2DSize{_width * factorX, _height * factorY};
    }

private:
    /// Ширина размера (интерпретируется как компонент X).
    _type _width = 0;

    /// Высота размера (интерпретируется как компонент Y).
    _type _height = 0;
};

// --- внешние псевдонимы типов ---
/// Список размеров (значения), используемый для хранения коллекций 2D-размеров.
using FC2DSizeList = QList<FC2DSize>;

/// Список указателей на размеры, применяется при работе с полиморфными объектами.
using FCPtr2DSizeList = QList<FC2DSize*>;

/// @brief Предопределённый "ошибочный" размер со значениями (-1, -1).
/// Используется как признак ошибки или недопустимого состояния (аналог NaN для геометрии).
constexpr FC2DSize FC2DSizeError{-1, -1};

// Проверки компиляции (гарантируют тривиальность и предсказуемый размер в памяти)
static_assert(std::is_trivially_copyable_v<FC2DSize>, "FC2DSize должен быть тривиально копируемым типом");
static_assert(sizeof(FC2DSize) == 2 * sizeof(float), "Размер FC2DSize должен быть в точности 2 × float");

#endif // FC_2D_SIZE_H
