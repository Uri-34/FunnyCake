#ifndef FC_2D_CELL_H
#define FC_2D_CELL_H

#include <QtGlobal>
#include <QList>
#include <array>
#include <type_traits>

#include "FC2DPoint.h"
#include "FC2DSize.h"

/**
 * @brief Класс для представления двумерной ячейки (прямоугольной области) с заданной позицией и размером.
 *
 * Предоставляет функционал для работы с геометрией прямоугольника: проверка вхождения точек,
 * пересечения с другими ячейками, вычисление центра и углов, а также операции перемещения.
 * Класс спроектирован как тривиальный (trivially copyable) для эффективной работы в контейнерах
 * и возможности использования в constexpr-контекстах.
 */
class FC2DCell
{
public:
    /// Тип данных, используемый для хранения координат и размеров (по умолчанию — float).
    using _type = float;

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт ячейку с позицией (0,0) и нулевым размером.
    constexpr FC2DCell() noexcept = default;

    /// @brief Конструктор с параметрами.
    /// @param position Позиция левого верхнего угла ячейки.
    /// @param size Размеры ячейки (ширина и высота).
    constexpr FC2DCell(const FC2DPoint& position, const FC2DSize& size) noexcept
        : _position(position)
        , _size(size)
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FC2DCell(const FC2DCell&) noexcept = default;

    /// @brief Оператор присваивания (тривиальный).
    constexpr FC2DCell& operator=(const FC2DCell&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FC2DCell() = default;

    // --- геттеры ---
    /// @brief Возвращает позицию левого верхнего угла ячейки.
    /// @return Константная ссылка на объект позиции.
    [[nodiscard]] constexpr const FC2DPoint& position() const noexcept { return _position; }

    /// @brief Возвращает размеры ячейки.
    /// @return Константная ссылка на объект размера.
    [[nodiscard]] constexpr const FC2DSize& size() const noexcept { return _size; }

    /// @brief Возвращает X-координату левого верхнего угла.
    [[nodiscard]] constexpr _type x() noexcept { return _position.x(); }

    /// @brief Возвращает Y-координату левого верхнего угла.
    [[nodiscard]] constexpr _type y() noexcept { return _position.y(); }

    /// @brief Возвращает ширину ячейки.
    [[nodiscard]] constexpr _type width() const noexcept { return _size.width(); }

    /// @brief Возвращает высоту ячейки.
    [[nodiscard]] constexpr _type height() const noexcept { return _size.height(); }

    // --- сеттеры ---
    /// @brief Устанавливает новую позицию левого верхнего угла ячейки.
    /// @param p Новая позиция.
    constexpr void set(const FC2DPoint &p) noexcept { _position = p; }

    /// @brief Устанавливает новый размер ячейки.
    /// @param size Новый размер.
    constexpr void set(const FC2DSize &size) noexcept { _size = size; }

    /// @brief Перемещает ячейку на заданный вектор (dx, dy).
    /// @param dx Смещение по оси X.
    /// @param dy Смещение по оси Y.
    constexpr void move(_type dx, _type dy) noexcept { _position = _position.translated(dx, dy); }

    /// @brief Перемещает ячейку на заданный вектор (объект FC2DPoint).
    /// @param delta Вектор смещения.
    constexpr void move(const FC2DPoint &delta) noexcept { _position += delta; }

    // --- геометрия ---
    /// @brief Вычисляет центр ячейки.
    /// @return Точка, расположенная в центре прямоугольника.
    [[nodiscard]] constexpr FC2DPoint center() const noexcept {
        return _position.translated(_size.width() / 2, _size.height() / 2);
    }

    /// @brief Возвращает координаты четырёх углов ячейки в порядке:
    ///        [левый-верхний, правый-верхний, левый-нижний, правый-нижний].
    /// @return Массив из 4 точек типа FC2DPoint.
    [[nodiscard]] constexpr std::array<FC2DPoint, 4> corners() noexcept
    {
        _type x0 = _position.x();
        _type y0 = _position.y();
        _type x1 = x0 + _size.width();
        _type y1 = y0 + _size.height();
        return {{
            FC2DPoint{x0, y0}, // левый-верхний угол
            FC2DPoint{x1, y0}, // правый-верхний угол
            FC2DPoint{x0, y1}, // левый-нижний угол
            FC2DPoint{x1, y1}  // правый-нижний угол
        }};
    }

    /// @brief Проверяет, находится ли точка внутри ячейки (включая границы).
    /// @param p Проверяемая точка.
    /// @return true, если точка принадлежит ячейке; иначе — false.
    [[nodiscard]] constexpr bool contains(FC2DPoint &p) noexcept
    {
        return p.x() >= _position.x() && p.x() <= _position.x() + _size.width() &&
               p.y() >= _position.y() && p.y() <= _position.y() + _size.height();
    }

    /// @brief Проверяет пересечение текущей ячейки с другой ячейкой.
    /// @param other Другая ячейка для проверки пересечения.
    /// @return true, если ячейки пересекаются (включая касание границами); иначе — false.
    [[nodiscard]] constexpr bool intersects(FC2DCell &other) noexcept
    {
        return !(_position.x() + _size.width()  < other._position.x() ||
                 other._position.x() + other._size.width()  < _position.x() ||
                 _position.y() + _size.height() < other._position.y() ||
                 other._position.y() + other._size.height() < _position.y());
    }

    /// @brief Проверяет, является ли ячейка пустой (нулевая ширина или высота).
    /// @return true, если ширина или высота <= 0; иначе — false.
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return _size.isEmpty(); }

    // --- операторы ---
    /// @brief Оператор сравнения на равенство.
    /// @param o Другая ячейка для сравнения.
    /// @return true, если позиции и размеры идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC2DCell &other) const noexcept { return _position == other._position && _size == other._size; }

    /// @brief Оператор сравнения на неравенство.
    /// @param o Другая ячейка для сравнения.
    /// @return true, если ячейки различаются по позиции или размеру; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC2DCell& o) const noexcept {
        return !(*this == o);
    }

private:
    /// Позиция левого верхнего угла ячейки.
    FC2DPoint _position;

    /// Размеры ячейки (ширина и высота). Инициализируется нулевым значением по умолчанию.
    FC2DSize  _size{};
};

// --- внешние псевдонимы типов ---
/// Список ячеек (значения), используемый для хранения коллекций ячеек.
using FC2DCellList = QList<FC2DCell>;

/// Список указателей на ячейки, применяется при работе с полиморфными объектами или динамическим выделением.
using FC2DPtrCellList = QList<FC2DCell*>;

/// @brief Возвращает предопределённую "ошибочную" ячейку с координатами и размерами (-1, -1).
/// Используется как признак ошибки или недопустимого состояния (аналог NaN для геометрии).
constexpr FC2DCell FC2DCellError {{-1, -1}, {-1, -1}};

// Примечание: для гарантии эффективности копирования и использования в низкоуровневых контекстах
// рекомендуется раскомментировать проверку тривиальности при условии, что FC2DPoint и FC2DSize
// также являются тривиальными типами:
// static_assert(std::is_trivially_copyable_v<FC2DCell>, "FC2DCell должен быть тривиально копируемым типом");

#endif // FC_2D_CELL_H
