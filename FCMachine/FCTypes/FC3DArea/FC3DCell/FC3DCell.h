#ifndef FC_3D_CELL_H
#define FC_3D_CELL_H

#include <QtGlobal>
#include <QList>
#include <type_traits>
#include <array>

#include "FC3DPoint.h"
#include "FC3DSize.h"

/**
 * @brief Класс для представления трёхмерной ячейки (прямоугольного параллелепипеда) с заданной позицией и размером.
 *
 * Семантика:
 *   - Позиция (_position) — координаты "якорной" точки ячейки (левый-верхний-ближний угол в правосторонней СК),
 *   - Размер (_size) — ширина × высота × глубина (компоненты должны быть ≥ 0 в корректной ячейке).
 *
 * Особенности:
 *   - Не наследует FC3DPoint/FC3DSize — чёткое разделение ответственности между сущностями,
 *   - Полная поддержка constexpr и noexcept для использования в высокопроизводительных и embedded-сценариях,
 *   - Предоставляет методы для геометрических операций: проверка вхождения точек, пересечения ячеек,
 *     вычисление центра и всех 8 углов, определение объёма.
 *
 * @note Класс спроектирован как тривиальный (trivially copyable) для эффективной работы в контейнерах
 *       и возможности размещения в памяти без дополнительных накладных расходов.
 */
class FC3DCell
{
public:
    /// Тип данных, используемый для хранения координат и размеров (алиас qreal для совместимости с Qt).
    using _type = float;

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт ячейку с позицией (0, 0, 0) и нулевым размером (0, 0, 0).
    constexpr FC3DCell() noexcept = default;

    /// @brief Конструктор с параметрами.
    /// @param position Позиция якорной точки ячейки (левый-верхний-ближний угол).
    /// @param size Размеры ячейки (ширина, высота, глубина).
    constexpr FC3DCell(const FC3DPoint& position, const FC3DSize& size) noexcept
        : _position(position)
        , _size(size)
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FC3DCell(const FC3DCell&) noexcept = default;

    /// @brief Оператор присваивания копированием (тривиальный).
    constexpr FC3DCell& operator=(const FC3DCell&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FC3DCell() = default;

    // --- геттеры ---
    /// @brief Возвращает позицию якорной точки ячейки.
    /// @return Константная ссылка на объект позиции.
    [[nodiscard]] constexpr const FC3DPoint& position() const noexcept { return _position; }

    /// @brief Возвращает размеры ячейки.
    /// @return Константная ссылка на объект размера.
    [[nodiscard]] constexpr const FC3DSize& size() const noexcept { return _size; }

    // --- прокси-геттеры для удобства доступа ---
    /// @brief Возвращает X-координату якорной точки ячейки.
    [[nodiscard]] constexpr _type x() const noexcept { return _position.x(); }

    /// @brief Возвращает Y-координату якорной точки ячейки.
    [[nodiscard]] constexpr _type y() const noexcept { return _position.y(); }

    /// @brief Возвращает Z-координату якорной точки ячейки.
    [[nodiscard]] constexpr _type z() const noexcept { return _position.z(); }

    /// @brief Возвращает ширину ячейки (размер по оси X).
    [[nodiscard]] constexpr _type width() const noexcept { return _size.width(); }

    /// @brief Возвращает высоту ячейки (размер по оси Y).
    [[nodiscard]] constexpr _type height() const noexcept { return _size.height(); }

    /// @brief Возвращает глубину ячейки (размер по оси Z).
    [[nodiscard]] constexpr _type depth() const noexcept { return _size.depth(); }

    // --- сеттеры ---
    /// @brief Устанавливает новую позицию якорной точки ячейки.
    /// @param position Новая позиция.
    constexpr void setPosition(const FC3DPoint& position) noexcept { _position = position; }

    /// @brief Устанавливает новый размер ячейки.
    /// @param size Новый размер.
    constexpr void setSize(const FC3DSize& size) noexcept { _size = size; }

    /// @brief Перемещает ячейку на заданный вектор (dx, dy, dz).
    /// @param dx Смещение по оси X.
    /// @param dy Смещение по оси Y.
    /// @param dz Смещение по оси Z.
    constexpr void move(_type dx, _type dy, _type dz) noexcept
    {
        _position = _position.translated(dx, dy, dz);
    }

    /// @brief Перемещает ячейку на заданный вектор (объект FC3DPoint).
    /// @param delta Вектор смещения.
    constexpr void move(const FC3DPoint& delta) noexcept { _position += delta; }

    // --- геометрические методы ---
    /// @brief Вычисляет центр ячейки.
    /// @return Точка, расположенная в центре прямоугольного параллелепипеда.
    [[nodiscard]] constexpr FC3DPoint center() const noexcept
    {
        return _position.translated(
            _size.width()  / 2,
            _size.height() / 2,
            _size.depth()  / 2
        );
    }

    /// @brief Возвращает координаты всех 8 углов ячейки.
    /// @return Массив из 8 точек в следующем порядке:
    ///         [0] (xmin, ymin, zmin) — левый-верхний-ближний,
    ///         [1] (xmax, ymin, zmin) — правый-верхний-ближний,
    ///         [2] (xmin, ymax, zmin) — левый-нижний-ближний,
    ///         [3] (xmax, ymax, zmin) — правый-нижний-ближний,
    ///         [4] (xmin, ymin, zmax) — левый-верхний-дальний,
    ///         [5] (xmax, ymin, zmax) — правый-верхний-дальний,
    ///         [6] (xmin, ymax, zmax) — левый-нижний-дальний,
    ///         [7] (xmax, ymax, zmax) — правый-нижний-дальний.
    [[nodiscard]] constexpr std::array<FC3DPoint, 8> corners() const noexcept
    {
        const _type x0 = _position.x();
        const _type y0 = _position.y();
        const _type z0 = _position.z();
        const _type x1 = x0 + _size.width();
        const _type y1 = y0 + _size.height();
        const _type z1 = z0 + _size.depth();

        return {{
            FC3DPoint{x0, y0, z0}, // 0: xmin, ymin, zmin
            FC3DPoint{x1, y0, z0}, // 1: xmax, ymin, zmin
            FC3DPoint{x0, y1, z0}, // 2: xmin, ymax, zmin
            FC3DPoint{x1, y1, z0}, // 3: xmax, ymax, zmin
            FC3DPoint{x0, y0, z1}, // 4: xmin, ymin, zmax
            FC3DPoint{x1, y0, z1}, // 5: xmax, ymin, zmax
            FC3DPoint{x0, y1, z1}, // 6: xmin, ymax, zmax
            FC3DPoint{x1, y1, z1}  // 7: xmax, ymax, zmax
        }};
    }

    /// @brief Проверяет, находится ли точка внутри ячейки (включая границы).
    /// @param point Проверяемая точка.
    /// @return true, если точка принадлежит ячейке (включая поверхности); иначе — false.
    [[nodiscard]] constexpr bool contains(const FC3DPoint& point) const noexcept
    {
        return point.x() >= _position.x() && point.x() <= _position.x() + _size.width()  &&
               point.y() >= _position.y() && point.y() <= _position.y() + _size.height() &&
               point.z() >= _position.z() && point.z() <= _position.z() + _size.depth();
    }

    /// @brief Проверяет пересечение текущей ячейки с другой ячейкой.
    /// @param other Другая ячейка для проверки пересечения.
    /// @return true, если ячейки пересекаются или касаются границами; иначе — false.
    [[nodiscard]] constexpr bool intersects(const FC3DCell& other) const noexcept
    {
        return !(_position.x() + _size.width()  < other._position.x() ||
                 other._position.x() + other._size.width()  < _position.x() ||
                 _position.y() + _size.height() < other._position.y() ||
                 other._position.y() + other._size.height() < _position.y() ||
                 _position.z() + _size.depth()  < other._position.z() ||
                 other._position.z() + other._size.depth()  < _position.z());
    }

    /// @brief Проверяет, является ли ячейка пустой (нулевой или отрицательный размер по любой оси).
    /// @return true, если ширина, высота или глубина ≤ 0; иначе — false.
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return _size.isEmpty(); }

    /// @brief Вычисляет объём ячейки.
    /// @return Произведение ширины × высоты × глубины.
    [[nodiscard]] constexpr _type volume() const noexcept { return _size.volume(); }

    // --- операторы сравнения ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другая ячейка для сравнения.
    /// @return true, если позиции и размеры идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC3DCell& other) const noexcept
    {
        return _position == other._position && _size == other._size;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другая ячейка для сравнения.
    /// @return true, если ячейки различаются по позиции или размеру; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC3DCell& other) const noexcept
    {
        return !(*this == other);
    }

private:
    /// Позиция якорной точки ячейки (левый-верхний-ближний угол).
    FC3DPoint _position{};

    /// Размеры ячейки (ширина, высота, глубина).
    FC3DSize _size{};
};

// --- внешние псевдонимы типов ---
/// Список ячеек (значения), используемый для хранения коллекций 3D-ячеек.
using FC3DCellList = QList<FC3DCell>;

/// Список указателей на ячейки, применяется при работе с полиморфными объектами или динамическим выделением.
using FC3DPtrCellList = QList<FC3DCell*>;

/// @brief Предопределённая "ошибочная" ячейка с недопустимыми координатами и размерами (-1, -1, -1).
/// Используется как признак ошибки или недопустимого состояния (аналог NaN для 3D-геометрии).
constexpr FC3DCell FC3DCellError{{-1, -1, -1}, {-1, -1, -1}};

// Проверки компиляции (гарантируют тривиальность и предсказуемый размер в памяти)
static_assert(std::is_trivially_copyable_v<FC3DCell>, "FC3DCell должен быть тривиально копируемым типом");

#endif // FC_3D_CELL_H
