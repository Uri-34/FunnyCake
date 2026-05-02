#ifndef FC_3D_AREA_H
#define FC_3D_AREA_H

#include <QtGlobal>
#include <QList>
#include <QVector>
#include <cmath>
#include <algorithm>
#include <type_traits>

#include "FC3DSize.h"
#include "FC3DCell.h"
#include "FC3DPoint.h"

/**
 * @brief Класс для представления трёхмерной области, разбитой на регулярную сетку ячеек.
 *
 * Область определяется тремя параметрами:
 *   - startPoint — координаты начальной точки области (левый-нижний-ближний угол в правосторонней СК),
 *   - areaSize   — полный размер области (ширина × высота × глубина),
 *   - cellCounts — количество ячеек по осям X, Y, Z (Nx, Ny, Nz), минимум 1×1×1.
 *
 * Каждая ячейка имеет одинаковый размер: (areaSize.width / Nx, areaSize.height / Ny, areaSize.depth / Nz).
 * Класс предоставляет доступ к ячейкам по индексам, координатам точки, а также поддерживает
 * кастомный порядок обхода ячеек для алгоритмов сканирования и трассировки.
 *
 * @note Класс нетривиален из-за использования QVector для хранения порядка обхода,
 *       поэтому не все методы могут быть constexpr.
 * @warning Система координат: правосторонняя, ось X — вправо, ось Y — вверх, ось Z — от наблюдателя.
 *          Начальная точка — левый-нижний-ближний угол области.
 *          Для плоттеров: X — вправо, Y — вперёд, Z — вниз.
 */
class FC3DArea
{
public:
    /// Тип данных, используемый для арифметических операций (для согласованности с геометрическими примитивами).
    using _type = qreal;

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт область 1×1×1 ячейку размером 1×1×1 с началом в (0, 0, 0).
    FC3DArea() noexcept
        : _startPoint{0, 0, 0}
        , _areaSize{1, 1, 1}
        , _cellCounts{1, 1, 1}
    {}

    /// @brief Конструктор с параметрами (базовый).
    /// @param startPoint Начальная точка области (левый-нижний-ближний угол).
    /// @param areaSize   Размер всей области (по умолчанию 1×1×1).
    /// @param cellCounts Количество ячеек по осям (Nx, Ny, Nz). Значения < 1 автоматически ограничиваются до 1.
    /// @note Координаты и размеры используют тип float, количество ячеек — целочисленные значения,
    ///       хранящиеся во float-полях FC3DPoint для совместимости с геометрическими примитивами.
    FC3DArea(const FC3DPoint& startPoint,
             const FC3DSize& areaSize = FC3DSize{1, 1, 1},
             const FC3DPoint& cellCounts = FC3DPoint{1, 1, 1}) noexcept
        : _startPoint{startPoint}
        , _areaSize{areaSize}
        , _cellCounts{
            std::max(qreal(1.0f), cellCounts.x()),
            std::max(qreal(1.0f), cellCounts.y()),
            std::max(qreal(1.0f), cellCounts.z())
          }
    {}

    /// @brief Конструктор для плоттеров с параметрами рабочей зоны.
    /// @param width  Ширина области по оси X (вправо).
    /// @param depth  Глубина области по оси Y (вперёд).
    /// @param height Высота области по оси Z (вниз).
    /// @param minZ   Минимальная координата Z (верхняя граница, численно меньше).
    /// @param maxZ   Максимальная координата Z (нижняя граница, численно больше).
    /// @param cellCounts Количество ячеек по осям (Nx, Ny, Nz). По умолчанию 1×1×1.
    /// @note Система координат плоттера: X — вправо, Y — вперёд, Z — вниз.
    ///       Начальная точка устанавливается в (0, 0, minZ).
    FC3DArea(qreal width, qreal depth, qreal height, qreal minZ, qreal maxZ,
             const FC3DPoint& cellCounts = FC3DPoint{1, 1, 1}) noexcept
        : _startPoint{0, 0, static_cast<float>(minZ)}
        , _areaSize{
            static_cast<float>(width),
            static_cast<float>(depth),
            static_cast<float>(height)
          }
        , _cellCounts{
              std::max(qreal(1.0f), cellCounts.x()),
              std::max(qreal(1.0f), cellCounts.y()),
              std::max(qreal(1.0f), cellCounts.z())
          }
    {
        Q_UNUSED(maxZ); // maxZ используется для валидации в вызывающем коде
    }

    /// @brief Конструктор копирования.
    FC3DArea(const FC3DArea&) = default;

    /// @brief Оператор присваивания копированием.
    FC3DArea& operator=(const FC3DArea&) = default;

    /// @brief Деструктор.
    ~FC3DArea() = default;

    // --- геттеры ---
    /// @brief Возвращает полный размер области.
    /// @return Размер области (ширина × высота × глубина).
    [[nodiscard]] const FC3DSize& size() const noexcept { return _areaSize; }

    /// @brief Возвращает начальную точку области (левый-нижний-ближний угол).
    /// @return Координаты начала области.
    [[nodiscard]] FC3DPoint startPoint() const noexcept { return _startPoint; }

    /// @brief Возвращает количество ячеек по осям (Nx, Ny, Nz).
    /// @return Точка, где x = количество ячеек по X, y = количество ячеек по Y, z = количество ячеек по Z.
    [[nodiscard]] FC3DPoint cellCounts() const noexcept { return _cellCounts; }

    /// @brief Возвращает количество ячеек по оси X (Nx).
    /// @return Целое число ≥ 1.
    [[nodiscard]] int cellCountX() const noexcept { return static_cast<int>(_cellCounts.x()); }

    /// @brief Возвращает количество ячеек по оси Y (Ny).
    /// @return Целое число ≥ 1.
    [[nodiscard]] int cellCountY() const noexcept { return static_cast<int>(_cellCounts.y()); }

    /// @brief Возвращает количество ячеек по оси Z (Nz).
    /// @return Целое число ≥ 1.
    [[nodiscard]] int cellCountZ() const noexcept { return static_cast<int>(_cellCounts.z()); }

    /// @brief Возвращает общее количество ячеек в области (Nx × Ny × Nz).
    /// @return Произведение количества ячеек по всем трём осям.
    [[nodiscard]] int totalCells() const noexcept { return cellCountX() * cellCountY() * cellCountZ(); }

    /// @brief Вычисляет размер одной ячейки (равномерное деление области на сетку).
    /// @return Размер ячейки: (areaSize.width / Nx, areaSize.height / Ny, areaSize.depth / Nz).
    [[nodiscard]] FC3DSize cellSize() const noexcept
    {
        return FC3DSize{
            _areaSize.width()  / _cellCounts.x(),
            _areaSize.height() / _cellCounts.y(),
            _areaSize.depth()  / _cellCounts.z()
        };
    }

    /// @brief Проверяет, является ли область пустой (нулевое или отрицательное количество ячеек).
    /// @return true, если общее количество ячеек ≤ 0; иначе — false.
    [[nodiscard]] bool isEmpty() const noexcept { return totalCells() <= 0; }

    // --- управление порядком обхода ячеек ---
    /// @brief Устанавливает кастомный порядок обхода ячеек по линейным индексам.
    /// @param order Вектор линейных индексов ячеек (idx = i + j*Nx + k*Nx*Ny) в желаемом порядке обхода.
    /// @note Если порядок не задан (пустой вектор), используется последовательный обход 0, 1, 2, ..., N-1.
    void setTraversalOrder(const QVector<int>& order)
    {
        _traversalOrder = order;
        _traversalIndex = 0;
    }

    /// @brief Сбрасывает текущую позицию обхода на начало.
    void resetTraversal() noexcept { _traversalIndex = 0; }

    /// @brief Возвращает размер текущего порядка обхода.
    /// @return Количество элементов в кастомном порядке, либо totalCells(), если порядок не задан.
    [[nodiscard]] int traversalSize() const noexcept
    {
        return _traversalOrder.isEmpty() ? totalCells() : _traversalOrder.size();
    }

    /// @brief Возвращает следующую ячейку в порядке обхода и продвигает индекс.
    /// @return Ячейка по текущему индексу обхода, либо FC3DCellError при выходе за границы.
    /// @note При первом вызове возвращает первую ячейку в порядке, при последующих — следующие.
    ///       После завершения обхода всегда возвращает FC3DCellError.
    [[nodiscard]] FC3DCell nextInTraversal()
    {
        if (_traversalOrder.isEmpty())
        {
            // Режим по умолчанию: последовательный обход
            if (_traversalIndex < 0 || _traversalIndex >= totalCells())
            {
                return FC3DCellError;
            }
            FC3DCell cell = atLinear(_traversalIndex);
            ++_traversalIndex;
            return cell;
        }
        else
        {
            // Кастомный порядок обхода
            if (_traversalIndex < 0 || _traversalIndex >= _traversalOrder.size())
            {
                return FC3DCellError;
            }
            int linearIdx = _traversalOrder[_traversalIndex];
            ++_traversalIndex;
            return atLinear(linearIdx);
        }
    }

    /// @brief Возвращает текущую позицию в порядке обхода.
    /// @return Индекс следующей ячейки (0-based), либо -1 если обход завершён или индекс недопустим.
    [[nodiscard]] int currentTraversalIndex() const noexcept
    {
        if (_traversalOrder.isEmpty())
        {
            return (_traversalIndex >= 0 && _traversalIndex <= totalCells())
                ? _traversalIndex : -1;
        }
        else
        {
            return (_traversalIndex >= 0 && _traversalIndex <= _traversalOrder.size())
                ? _traversalIndex : -1;
        }
    }

    /// @brief Проверяет, завершён ли обход всех ячеек.
    /// @return true, если достигнут конец порядка обхода; иначе — false.
    [[nodiscard]] bool isTraversalFinished() const noexcept
    {
        return _traversalOrder.isEmpty()
            ? _traversalIndex >= totalCells()
            : _traversalIndex >= _traversalOrder.size();
    }

    // --- доступ к ячейкам ---
    /// @brief Возвращает ячейку по линейному индексу (row-major порядок: idx = i + j*Nx + k*Nx*Ny).
    /// @param linearIndex Линейный индекс ячейки в диапазоне [0, totalCells() - 1].
    /// @return Ячейка с вычисленной позицией и размером, либо FC3DCellError при некорректном индексе.
    [[nodiscard]] FC3DCell atLinear(int linearIndex) const noexcept
    {
        if (linearIndex < 0 || linearIndex >= totalCells())
        {
            return FC3DCellError;
        }
        int Nx = cellCountX();
        int Ny = cellCountY();
        int k = linearIndex / (Nx * Ny);
        int rem = linearIndex % (Nx * Ny);
        int j = rem / Nx;
        int i = rem % Nx;
        return at(i, j, k);
    }

    /// @brief Возвращает ячейку по трёхмерным индексам (i, j, k).
    /// @param i Индекс ячейки по оси X (столбец), диапазон [0, Nx - 1].
    /// @param j Индекс ячейки по оси Y (строка), диапазон [0, Ny - 1].
    /// @param k Индекс ячейки по оси Z (слой), диапазон [0, Nz - 1].
    /// @return Ячейка с вычисленной позицией и размером, либо FC3DCellError при выходе за границы сетки.
    [[nodiscard]] FC3DCell at(int i, int j, int k) const noexcept;

    /// @brief Возвращает ячейку, содержащую заданную точку в 3D-пространстве.
    /// @param point Координаты точки в глобальной системе.
    /// @return Ячейка, содержащая точку (включая границы), либо FC3DCellError,
    ///         если точка находится вне области.
    /// @note Добавлена защита от погрешностей округления на правой/верхней/дальней границах.
    [[nodiscard]] FC3DCell at(const FC3DPoint& point) const noexcept;

    /// @brief Возвращает ячейку по индексам (i, j) в базовом слое Z=0.
    /// @param i Индекс по оси X.
    /// @param j Индекс по оси Y.
    /// @return Ячейка в слое Z=0 с указанными индексами.
    [[nodiscard]] FC3DCell atXY(int i, int j) const noexcept { return at(i, j, 0); }

    // --- вспомогательные методы ---
    /// @brief Вычисляет индексы ячейки (i, j, k), содержащей заданную точку.
    /// @param point Координаты точки в глобальной системе.
    /// @return Точка с индексами (i, j, k), либо FC3DPointError, если точка вне области.
    /// @note Добавлена защита от погрешностей округления на границах.
    [[nodiscard]] FC3DPoint indexAt(const FC3DPoint& point) const noexcept;

    /// @brief Проверяет, принадлежит ли точка области (включая границы).
    /// @param point Проверяемая точка.
    /// @return true, если точка находится внутри или на границе области; иначе — false.
    /// @note Точка с координатами равными правой/верхней/дальней границе считается принадлежащей области.
    [[nodiscard]] constexpr bool contains(const FC3DPoint& point) const noexcept
    {
        if (point == FC3DPointError)
        {
            return false;
        }
        return point.x() >= _startPoint.x() && point.x() <= _startPoint.x() + _areaSize.width()  &&
               point.y() >= _startPoint.y() && point.y() <= _startPoint.y() + _areaSize.height() &&
               point.z() >= _startPoint.z() && point.z() <= _startPoint.z() + _areaSize.depth();
    }

    // --- операторы сравнения ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другая область для сравнения.
    /// @return true, если startPoint, areaSize и cellCounts идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC3DArea& other) const noexcept
    {
        return _startPoint == other._startPoint &&
               _areaSize == other._areaSize &&
               _cellCounts == other._cellCounts;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другая область для сравнения.
    /// @return true, если области различаются хотя бы по одному параметру; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC3DArea& other) const noexcept
    {
        return !(*this == other);
    }

private:
    /// Начальная точка области (левый-нижний-ближний угол в правосторонней системе координат).
    FC3DPoint _startPoint;

    /// Полный размер области (ширина × высота × глубина).
    FC3DSize _areaSize;

    /// Количество ячеек по осям (Nx, Ny, Nz), хранится как точка для совместимости с геометрическими операциями.
    /// Значения всегда ≥ 1.0.
    FC3DPoint _cellCounts;

    /// Кастомный порядок обхода ячеек по линейным индексам (опционально).
    /// Если пуст — используется последовательный обход.
    QVector<int> _traversalOrder;

    /// Текущая позиция в порядке обхода (индекс следующей ячейки).
    int _traversalIndex = 0;
};

// --- внешние псевдонимы типов ---
/// Список областей (значения), используемый для хранения коллекций 3D-областей.
using FC3DAreaList = QList<FC3DArea>;

/// Список указателей на области, применяется при работе с полиморфными объектами.
using FC3DPtrAreaList = QList<FC3DArea*>;

/// @brief Возвращает предопределённую "ошибочную" область с недопустимыми параметрами.
/// @return Область с startPoint=(-1,-1,-1), areaSize=(-1,-1,-1), cellCounts=(-1,-1,-1).
/// @note Реализовано как функция (а не статическая переменная), так как класс содержит
///       нетривиальные члены (QVector), что запрещает статическую инициализацию в заголовочном файле.
[[nodiscard]] inline FC3DArea FC3DAreaError() noexcept
{
    static const FC3DArea error{FC3DPoint{-1, -1, -1}, FC3DSize{-1, -1, -1}, FC3DPoint{-1, -1, -1}};
    return error;
}

#endif // FC_3D_AREA_H
