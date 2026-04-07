#ifndef FC_2D_POINT_H
#define FC_2D_POINT_H

#include <QtGlobal>
#include <QList>
#include <type_traits>
#include <cmath>

class FC2DSize;
class FC3DSize;
class FC3DPoint;

/**
 * @brief Класс для представления двумерной точки с координатами (x, y).
 *
 * Предоставляет функционал для геометрических операций: вычисление расстояний,
 * углов, нормализация векторов, преобразования координат и арифметические операции.
 * Класс спроектирован как тривиальный (trivially copyable) для эффективной работы
 * в контейнерах и возможности использования в constexpr-контекстах (требуется C++23
 * для методов с тригонометрическими функциями).
 */
class FC2DPoint
{
public:
    /// Тип данных, используемый для хранения координат (по умолчанию — float).
    using _type = float;

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт точку с координатами (0, 0).
    constexpr FC2DPoint() noexcept = default;

    /// @brief Конструктор с параметрами.
    /// @param x Координата по оси X.
    /// @param y Координата по оси Y.
    constexpr FC2DPoint(_type x, _type y) noexcept
        : _x(x)
        , _y(y)
    {}

    /// @brief Конструктор преобразования из 3D-точки (координата Z игнорируется).
    /// @param point Исходная 3D-точка.
    explicit constexpr FC2DPoint(FC3DPoint& point) noexcept;

    /// @brief Конструктор копирования (тривиальный).
    constexpr FC2DPoint(const FC2DPoint&) noexcept = default;

    /// @brief Оператор присваивания копированием (тривиальный).
    constexpr FC2DPoint& operator=(const FC2DPoint&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FC2DPoint() = default;

    // --- геттеры ---
    /// @brief Возвращает координату X точки.
    /// @return Значение координаты по оси X.
    [[nodiscard]] constexpr _type x() const noexcept { return _x; }

    /// @brief Возвращает координату Y точки.
    /// @return Значение координаты по оси Y.
    [[nodiscard]] constexpr _type y() const noexcept { return _y; }

    // --- сеттеры ---
    /// @brief Устанавливает новое значение координаты X.
    /// @param x Новое значение координаты по оси X.
    constexpr void setX(_type x) noexcept { _x = x; }

    /// @brief Устанавливает новое значение координаты Y.
    /// @param y Новое значение координаты по оси Y.
    constexpr void setY(_type y) noexcept { _y = y; }

    /// @brief Устанавливает обе координаты одновременно.
    /// @param x Новое значение координаты по оси X.
    /// @param y Новое значение координаты по оси Y.
    constexpr void setXY(_type x, _type y) noexcept { _x = x; _y = y; }

    // --- преобразования ---
    /// @brief Преобразует 2D-точку в 3D-точку, добавляя координату Z.
    /// @param z Значение координаты по оси Z (по умолчанию 0).
    /// @return 3D-точка с сохранёнными X, Y и заданным Z.
    [[nodiscard]] constexpr FC3DPoint to3D(_type z = 0) const noexcept;

    /// @brief Преобразует точку в размер, интерпретируя координаты как ширину и высоту.
    /// @return Объект размера, где width = x, height = y.
    [[nodiscard]] constexpr FC2DSize to2DSize() const noexcept;

    [[nodiscard]] constexpr FC2DPoint from2DSize(FC2DSize& size) noexcept;

    // --- геометрические методы ---
    /// @brief Вычисляет евклидово расстояние до другой точки.
    /// @param other Другая точка.
    /// @return Расстояние между точками.
    /// @note Требует C++23 для constexpr-контекста из-за std::hypot.
    [[nodiscard]] /*constexpr*/ _type distanceTo(const FC2DPoint& other) const noexcept
    {
        const _type dx = _x - other._x;
        const _type dy = _y - other._y;
        return std::hypot(dx, dy);
    }

    /// @brief Вычисляет квадрат расстояния до другой точки (без извлечения корня).
    /// @param other Другая точка.
    /// @return Квадрат расстояния между точками.
    /// @note Более производительная альтернатива distanceTo() для сравнений.
    [[nodiscard]] constexpr _type distanceSquaredTo(const FC2DPoint& other) const noexcept
    {
        const _type dx = _x - other._x;
        const _type dy = _y - other._y;
        return dx * dx + dy * dy;
    }

    /// @brief Вычисляет угол вектора (от начала координат к точке) относительно оси X.
    /// @return Угол в радианах в диапазоне [-π, π].
    /// @note Требует C++23 для constexpr-контекста из-за std::atan2.
    [[nodiscard]] /*constexpr*/ _type angle() const noexcept { return std::atan2(_y, _x); }

    /// @brief Вычисляет угол между двумя векторами (от начала координат к this и other).
    /// @param other Другая точка (конец второго вектора).
    /// @return Угол в радианах в диапазоне [0, π].
    /// @note Требует C++23 для constexpr-контекста из-за std::atan2.
    [[nodiscard]] /*constexpr*/ _type angleTo(const FC2DPoint& other) const noexcept
    {
        const _type dot = _x * other._x + _y * other._y;
        const _type cross = _x * other._y - _y * other._x;
        return std::atan2(std::abs(cross), dot);
    }

    /// @brief Вычисляет длину вектора (расстояние от начала координат до точки).
    /// @return Длина вектора.
    /// @note Требует C++23 для constexpr-контекста из-за std::hypot.
    [[nodiscard]] /*constexpr*/ _type length() const noexcept { return std::hypot(_x, _y); }

    /// @brief Вычисляет квадрат длины вектора (без извлечения корня).
    /// @return Квадрат длины вектора.
    /// @note Более производительная альтернатива length() для сравнений.
    [[nodiscard]] constexpr _type lengthSquared() const noexcept { return _x * _x + _y * _y; }

    /// @brief Возвращает нормализованный вектор (единичной длины).
    /// @return Точка с координатами, нормализованными до длины 1.
    ///        Если исходная длина равна 0 — возвращает точку (0, 0).
    [[nodiscard]] FC2DPoint normalized() const noexcept
    {
        const _type l = length();
        if (l == 0) return FC2DPoint{};
        const _type inv = 1.0f / l;
        return FC2DPoint{_x * inv, _y * inv};
    }

    /// @brief Возвращает точку, смещённую на заданный вектор (dx, dy).
    /// @param dx Смещение по оси X.
    /// @param dy Смещение по оси Y.
    /// @return Новая точка с прибавленными координатами.
    [[nodiscard]] constexpr FC2DPoint translated(_type dx, _type dy) const noexcept
    {
        return FC2DPoint{_x + dx, _y + dy};
    }

    /// @brief Возвращает точку, смещённую на вектор, заданный другой точкой.
    /// @param delta Вектор смещения (координаты интерпретируются как dx, dy).
    /// @return Новая точка с прибавленными координатами.
    [[nodiscard]] constexpr FC2DPoint translated(const FC2DPoint& delta) const noexcept
    {
        return FC2DPoint{_x + delta._x, _y + delta._y};
    }

    /// @brief Возвращает точку, масштабированную относительно начала координат.
    /// @param scaleX Коэффициент масштабирования по оси X.
    /// @param scaleY Коэффициент масштабирования по оси Y.
    /// @return Новая точка с умноженными координатами.
    [[nodiscard]] constexpr FC2DPoint scaled(_type scaleX, _type scaleY) const noexcept
    {
        return FC2DPoint{_x * scaleX, _y * scaleY};
    }

    /// @brief Возвращает точку, масштабированную относительно начала координат с одинаковым коэффициентом.
    /// @param factor Единый коэффициент масштабирования для обеих осей.
    /// @return Новая точка с умноженными координатами.
    [[nodiscard]] constexpr FC2DPoint scaled(_type factor) const noexcept
    {
        return FC2DPoint{_x * factor, _y * factor};
    }

    /// @brief Возвращает точку, повёрнутую вокруг начала координат.
    /// @param angle Угол поворота в радианах (положительное значение — против часовой стрелки).
    /// @return Новая точка после поворота.
    /// @note Требует C++23 для constexpr-контекста из-за std::cos/std::sin.
    [[nodiscard]] /*constexpr*/ FC2DPoint rotated(_type angle) const noexcept
    {
        const _type cosA = std::cos(angle);
        const _type sinA = std::sin(angle);
        return FC2DPoint{
            _x * cosA - _y * sinA,
            _x * sinA + _y * cosA
        };
    }

    /// @brief Возвращает точку, отражённую относительно оси X (инвертирует Y).
    /// @return Новая точка с координатой y = -y.
    [[nodiscard]] constexpr FC2DPoint mirroredX() const noexcept { return FC2DPoint{_x, -_y}; }

    /// @brief Возвращает точку, отражённую относительно оси Y (инвертирует X).
    /// @return Новая точка с координатой x = -x.
    [[nodiscard]] constexpr FC2DPoint mirroredY() const noexcept { return FC2DPoint{-_x, _y}; }

    /// @brief Возвращает точку, отражённую относительно начала координат.
    /// @return Новая точка с координатами (-x, -y).
    [[nodiscard]] constexpr FC2DPoint mirroredOrigin() const noexcept { return -(*this); }

    // --- операторы ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другая точка для сравнения.
    /// @return true, если обе координаты совпадают; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC2DPoint& other) const noexcept
    {
        return _x == other._x && _y == other._y;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другая точка для сравнения.
    /// @return true, если точки различаются хотя бы по одной координате; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC2DPoint& other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Оператор сложения точек (покомпонентное сложение).
    /// @param other Точка-слагаемое.
    /// @return Новая точка с суммой координат.
    [[nodiscard]] constexpr FC2DPoint operator+(const FC2DPoint& other) const noexcept
    {
        return FC2DPoint{_x + other._x, _y + other._y};
    }

    /// @brief Оператор вычитания точек (покомпонентное вычитание).
    /// @param other Вычитаемая точка.
    /// @return Новая точка с разностью координат.
    [[nodiscard]] constexpr FC2DPoint operator-(const FC2DPoint& other) const noexcept
    {
        return FC2DPoint{_x - other._x, _y - other._y};
    }

    /// @brief Оператор составного сложения (+=).
    /// @param other Точка-слагаемое.
    /// @return Ссылка на текущий объект после модификации.
    constexpr FC2DPoint& operator+=(const FC2DPoint& other) noexcept
    {
        _x += other._x;
        _y += other._y;
        return *this;
    }

    /// @brief Оператор составного вычитания (-=).
    /// @param other Вычитаемая точка.
    /// @return Ссылка на текущий объект после модификации.
    constexpr FC2DPoint& operator-=(const FC2DPoint& other) noexcept
    {
        _x -= other._x;
        _y -= other._y;
        return *this;
    }

    /// @brief Унарный оператор минус (отражение относительно начала координат).
    /// @return Новая точка с инвертированными координатами (-x, -y).
    [[nodiscard]] constexpr FC2DPoint operator-() const noexcept
    {
        return FC2DPoint{-_x, -_y};
    }

    /// @brief Оператор умножения на скаляр.
    /// @param factor Множитель.
    /// @return Новая точка с координатами, умноженными на скаляр.
    [[nodiscard]] constexpr FC2DPoint operator*(_type factor) const noexcept
    {
        return scaled(factor);
    }

    /// @brief Оператор деления на скаляр.
    /// @param divisor Делитель (должен быть ненулевым).
    /// @return Новая точка с координатами, разделёнными на скаляр.
    [[nodiscard]] constexpr FC2DPoint operator/(_type divisor) const noexcept
    {
        const _type inv = 1.0f / divisor;
        return FC2DPoint{_x * inv, _y * inv};
    }

    /// @brief Вычисляет скалярное (векторное dot) произведение двух точек.
    /// @param other Другая точка (второй вектор).
    /// @return Скалярное произведение: x1*x2 + y1*y2.
    [[nodiscard]] constexpr _type dotProduct(const FC2DPoint& other) const noexcept
    {
        return _x * other._x + _y * other._y;
    }

    /// @brief Вычисляет псевдоскалярное (2D cross) произведение двух точек.
    /// @param other Другая точка (второй вектор).
    /// @return Значение: x1*y2 - y1*x2 (модуль равен площади параллелограмма).
    [[nodiscard]] constexpr _type crossProduct(const FC2DPoint& other) const noexcept
    {
        return _x * other._y - _y * other._x;
    }

private:
    /// Координата точки по оси X.
    _type _x = 0;

    /// Координата точки по оси Y.
    _type _y = 0;
};

/// @brief Предопределённая "ошибочная" точка с координатами (-1, -1).
/// Используется как признак ошибки или недопустимого состояния (аналог NaN для геометрии).
constexpr FC2DPoint FC2DPointError{-1, -1};

using FC2DPointVector = QVector<FC2DPoint>;

#endif // FC_2D_POINT_H
