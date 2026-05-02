#ifndef FC_3D_POINT_H
#define FC_3D_POINT_H

#include <QtGlobal>
#include <QList>
#include <type_traits>
#include <cmath>

#include "FC2DPoint.h"
#include "FC3DSize.h"

/**
 * @brief Класс для представления трёхмерной точки с координатами (x, y, z).
 *
 * Предоставляет функционал для геометрических операций в 3D-пространстве:
 * вычисление расстояний, повороты вокруг осей, смещения, масштабирование.
 * Класс спроектирован как полностью автономный (не наследует 2D-классы)
 * для чёткого разделения ответственности и предотвращения неоднозначностей.
 *
 * @note Класс является тривиальным (trivially copyable), что позволяет эффективно
 *       использовать его в контейнерах и встраиваемых системах (включая среды с ограничениями памяти).
 */
class FC3DPoint
{
public:
    /// Тип данных, используемый для хранения координат (по умолчанию — float).
    using _type = qreal;

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт точку с координатами (0, 0, 0).
    constexpr FC3DPoint() noexcept = default;

    /// @brief Конструктор с параметрами.
    /// @param x Координата по оси X.
    /// @param y Координата по оси Y.
    /// @param z Координата по оси Z.
    constexpr FC3DPoint(_type x, _type y, _type z) noexcept
        : _x(x)
        , _y(y)
        , _z(z)
    {}

    /// @brief Конструктор преобразования из 2D-точки с добавлением координаты Z.
    /// @param point Исходная 2D-точка (координаты интерпретируются как X и Y).
    /// @param z Значение координаты по оси Z (по умолчанию 0).
    explicit constexpr FC3DPoint(FC2DPoint& point, _type z = 0) noexcept
        : _x(point.x()),
          _y(point.y()),
          _z(z)
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FC3DPoint(const FC3DPoint&) noexcept = default;

    /// @brief Оператор присваивания копированием (тривиальный).
    constexpr FC3DPoint& operator=(const FC3DPoint&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FC3DPoint() = default;

    // --- геттеры ---
    /// @brief Возвращает координату X точки.
    /// @return Значение координаты по оси X.
    [[nodiscard]] constexpr _type x() const noexcept { return _x; }

    /// @brief Возвращает координату Y точки.
    /// @return Значение координаты по оси Y.
    [[nodiscard]] constexpr _type y() const noexcept { return _y; }

    /// @brief Возвращает координату Z точки.
    /// @return Значение координаты по оси Z.
    [[nodiscard]] constexpr _type z() const noexcept { return _z; }

    // --- сеттеры ---
    /// @brief Устанавливает новое значение координаты X.
    /// @param x Новое значение координаты по оси X.
    constexpr void setX(_type x) noexcept { _x = x; }

    /// @brief Устанавливает новое значение координаты Y.
    /// @param y Новое значение координаты по оси Y.
    constexpr void setY(_type y) noexcept { _y = y; }

    /// @brief Устанавливает новое значение координаты Z.
    /// @param z Новое значение координаты по оси Z.
    constexpr void setZ(_type z) noexcept { _z = z; }

    /// @brief Устанавливает все три координаты одновременно.
    /// @param x Новое значение координаты по оси X.
    /// @param y Новое значение координаты по оси Y.
    /// @param z Новое значение координаты по оси Z.
    constexpr void setXYZ(_type x, _type y, _type z) noexcept
    {
        _x = x;
        _y = y;
        _z = z;
    }

    // --- преобразования ---
    /// @brief Проекция точки на XY-плоскость (координата Z игнорируется).
    /// @return 2D-точка с координатами (x, y).
    [[nodiscard]] constexpr FC2DPoint to2D() const noexcept;

    /// @brief Преобразует точку в 3D-размер, интерпретируя координаты как компоненты размера.
    /// @return Объект размера, где width = x, height = y, depth = z.
    [[nodiscard]] constexpr FC3DSize toSize() const noexcept { return FC3DSize{_x, _y, _z}; }

    /// @brief Создаёт точку из 3D-размера, интерпретируя компоненты размера как координаты.
    /// @param size Исходный размер (компоненты интерпретируются как x = width, y = height, z = depth).
    /// @return Новая точка на основе компонентов размера.
    [[nodiscard]] static constexpr FC3DPoint fromSize(const FC3DSize& size) noexcept;

    // --- геометрические методы ---
    /// @brief Вычисляет евклидово расстояние до другой точки.
    /// @param other Другая точка.
    /// @return Расстояние между точками.
    /// @note Требует C++23 для constexpr-контекста из-за std::hypot.
    [[nodiscard]] /*constexpr*/ _type distanceTo(const FC3DPoint& other) const noexcept
    {
        const _type dx = _x - other._x;
        const _type dy = _y - other._y;
        const _type dz = _z - other._z;
        return std::hypot(dx, std::hypot(dy, dz));
    }

    /// @brief Вычисляет квадрат расстояния до другой точки (без извлечения корня).
    /// @param other Другая точка.
    /// @return Квадрат расстояния между точками.
    /// @note Более производительная альтернатива distanceTo() для сравнений.
    [[nodiscard]] constexpr _type distanceSquaredTo(const FC3DPoint& other) const noexcept
    {
        const _type dx = _x - other._x;
        const _type dy = _y - other._y;
        const _type dz = _z - other._z;
        return dx * dx + dy * dy + dz * dz;
    }

    /// @brief Возвращает точку, смещённую на заданный вектор (dx, dy, dz).
    /// @param dx Смещение по оси X.
    /// @param dy Смещение по оси Y.
    /// @param dz Смещение по оси Z.
    /// @return Новая точка с прибавленными координатами.
    [[nodiscard]] constexpr FC3DPoint translated(_type dx, _type dy, _type dz) const noexcept
    {
        return FC3DPoint{_x + dx, _y + dy, _z + dz};
    }

    /// @brief Возвращает точку, смещённую на вектор, заданный другой точкой.
    /// @param delta Вектор смещения (координаты интерпретируются как dx, dy, dz).
    /// @return Новая точка с прибавленными координатами.
    [[nodiscard]] constexpr FC3DPoint translated(const FC3DPoint& delta) const noexcept
    {
        return FC3DPoint{_x + delta._x, _y + delta._y, _z + delta._z};
    }

    /// @brief Возвращает точку, масштабированную относительно начала координат.
    /// @param scaleX Коэффициент масштабирования по оси X.
    /// @param scaleY Коэффициент масштабирования по оси Y.
    /// @param scaleZ Коэффициент масштабирования по оси Z.
    /// @return Новая точка с умноженными координатами.
    [[nodiscard]] constexpr FC3DPoint scaled(_type scaleX, _type scaleY, _type scaleZ) const noexcept
    {
        return FC3DPoint{_x * scaleX, _y * scaleY, _z * scaleZ};
    }

    /// @brief Возвращает точку, масштабированную относительно начала координат с одинаковым коэффициентом.
    /// @param factor Единый коэффициент масштабирования для всех осей.
    /// @return Новая точка с умноженными координатами.
    [[nodiscard]] constexpr FC3DPoint scaled(_type factor) const noexcept
    {
        return FC3DPoint{_x * factor, _y * factor, _z * factor};
    }

    /// @brief Возвращает точку, повёрнутую вокруг оси X.
    /// @param angle Угол поворота в радианах (положительное значение — против часовой стрелки при взгляде с +X).
    /// @return Новая точка после поворота.
    /// @note Требует C++23 для constexpr-контекста из-за std::cos/std::sin.
    [[nodiscard]] /*constexpr*/ FC3DPoint rotatedX(_type angle) const noexcept
    {
        const _type cosA = std::cos(angle);
        const _type sinA = std::sin(angle);
        return FC3DPoint{
            _x,
            _y * cosA - _z * sinA,
            _y * sinA + _z * cosA
        };
    }

    /// @brief Возвращает точку, повёрнутую вокруг оси Y.
    /// @param angle Угол поворота в радианах (положительное значение — против часовой стрелки при взгляде с +Y).
    /// @return Новая точка после поворота.
    /// @note Требует C++23 для constexpr-контекста из-за std::cos/std::sin.
    [[nodiscard]] /*constexpr*/ FC3DPoint rotatedY(_type angle) const noexcept
    {
        const _type cosA = std::cos(angle);
        const _type sinA = std::sin(angle);
        return FC3DPoint{
            _x * cosA + _z * sinA,
            _y,
            -_x * sinA + _z * cosA
        };
    }

    /// @brief Возвращает точку, повёрнутую вокруг оси Z.
    /// @param angle Угол поворота в радианах (положительное значение — против часовой стрелки при взгляде с +Z).
    /// @return Новая точка после поворота.
    /// @note Требует C++23 для constexpr-контекста из-за std::cos/std::sin.
    [[nodiscard]] /*constexpr*/ FC3DPoint rotatedZ(_type angle) const noexcept
    {
        const _type cosA = std::cos(angle);
        const _type sinA = std::sin(angle);
        return FC3DPoint{
            _x * cosA - _y * sinA,
            _x * sinA + _y * cosA,
            _z
        };
    }

    // --- операторы ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другая точка для сравнения.
    /// @return true, если все три координаты совпадают; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC3DPoint& other) const noexcept
    {
        return _x == other._x && _y == other._y && _z == other._z;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другая точка для сравнения.
    /// @return true, если точки различаются хотя бы по одной координате; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC3DPoint& other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Оператор сложения точек (покомпонентное сложение).
    /// @param other Точка-слагаемое.
    /// @return Новая точка с суммой координат.
    [[nodiscard]] constexpr FC3DPoint operator+(const FC3DPoint& other) const noexcept
    {
        return FC3DPoint{_x + other._x, _y + other._y, _z + other._z};
    }

    /// @brief Оператор вычитания точек (покомпонентное вычитание).
    /// @param other Вычитаемая точка.
    /// @return Новая точка с разностью координат.
    [[nodiscard]] constexpr FC3DPoint operator-(const FC3DPoint& other) const noexcept
    {
        return FC3DPoint{_x - other._x, _y - other._y, _z - other._z};
    }

    /// @brief Оператор составного сложения (+=).
    /// @param other Точка-слагаемое.
    /// @return Ссылка на текущий объект после модификации.
    constexpr FC3DPoint& operator+=(const FC3DPoint& other) noexcept
    {
        _x += other._x;
        _y += other._y;
        _z += other._z;
        return *this;
    }

    /// @brief Оператор составного вычитания (-=).
    /// @param other Вычитаемая точка.
    /// @return Ссылка на текущий объект после модификации.
    constexpr FC3DPoint& operator-=(const FC3DPoint& other) noexcept
    {
        _x -= other._x;
        _y -= other._y;
        _z -= other._z;
        return *this;
    }

    /// @brief Унарный оператор минус (инверсия всех координат).
    /// @return Новая точка с координатами (-x, -y, -z).
    [[nodiscard]] constexpr FC3DPoint operator-() const noexcept
    {
        return FC3DPoint{-_x, -_y, -_z};
    }

    /// @brief Оператор умножения на скаляр.
    /// @param factor Множитель.
    /// @return Новая точка с координатами, умноженными на скаляр.
    [[nodiscard]] constexpr FC3DPoint operator*(_type factor) const noexcept
    {
        return scaled(factor);
    }

    /// @brief Оператор деления на скаляр.
    /// @param divisor Делитель (должен быть ненулевым).
    /// @return Новая точка с координатами, разделёнными на скаляр.
    [[nodiscard]] constexpr FC3DPoint operator/(_type divisor) const noexcept
    {
        const _type inv = 1.0f / divisor;
        return FC3DPoint{_x * inv, _y * inv, _z * inv};
    }

    /// @brief Вычисляет скалярное (векторное dot) произведение двух точек.
    /// @param other Другая точка (второй вектор).
    /// @return Скалярное произведение: x1*x2 + y1*y2 + z1*z2.
    [[nodiscard]] constexpr _type dotProduct(const FC3DPoint& other) const noexcept
    {
        return _x * other._x + _y * other._y + _z * other._z;
    }

    /// @brief Вычисляет векторное (cross) произведение двух точек.
    /// @param other Другая точка (второй вектор).
    /// @return Векторное произведение: (y1*z2 - z1*y2, z1*x2 - x1*z2, x1*y2 - y1*x2).
    [[nodiscard]] constexpr FC3DPoint crossProduct(const FC3DPoint& other) const noexcept
    {
        return FC3DPoint{
            _y * other._z - _z * other._y,
            _z * other._x - _x * other._z,
            _x * other._y - _y * other._x
        };
    }

private:
    /// Координата точки по оси X.
    _type _x = 0;

    /// Координата точки по оси Y.
    _type _y = 0;

    /// Координата точки по оси Z.
    _type _z = 0;
};

// --- внешние псевдонимы типов ---
/// Список точек (значения), используемый для хранения коллекций 3D-точек.
using FC3DPointList = QList<FC3DPoint>;

/// Список указателей на точки, применяется при работе с полиморфными объектами.
using FC3DPtrPointList = QList<FC3DPoint*>;

/// @brief Предопределённая "ошибочная" точка с координатами (-1, -1, -1).
/// Используется как признак ошибки или недопустимого состояния (аналог NaN для 3D-геометрии).
constexpr FC3DPoint FC3DPointError{-1, -1, -1};

// Примечание: для гарантии эффективности копирования и использования в низкоуровневых контекстах
// рекомендуется раскомментировать проверку тривиальности при условии, что все специальные методы
// объявлены как = default:
// static_assert(std::is_trivially_copyable_v<FC3DPoint>, "FC3DPoint должен быть тривиально копируемым типом");
// static_assert(sizeof(FC3DPoint) == 3 * sizeof(_type), "Размер FC3DPoint должен быть в точности 3 × _type");

#endif // FC_3D_POINT_H
