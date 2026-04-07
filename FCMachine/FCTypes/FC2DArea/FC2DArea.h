#ifndef FC_2D_AREA_H
#define FC_2D_AREA_H

#include <QtGlobal>
#include <QList>
#include <QVector>
#include <cmath>
#include <type_traits>

#include "FC2DPoint.h"
#include "FC2DSize.h"
#include "FC2DCell.h"

/**
 * @brief Класс для представления двумерной области, разбитой на регулярную сетку ячеек.
 *
 * Область определяется тремя параметрами:
 *   - startPoint — координаты левого верхнего угла области,
 *   - areaSize   — полный размер области (ширина × высота),
 *   - cellCounts — количество ячеек по осям X и Y (Nx, Ny), минимум 1×1.
 *
 * Каждая ячейка имеет одинаковый размер: (areaSize.width / Nx, areaSize.height / Ny).
 * Класс предоставляет доступ к ячейкам по индексам, координатам точки, а также поддерживает
 * кастомный порядок обхода ячеек для алгоритмов сканирования.
 *
 * @note Класс нетривиален из-за использования QVector для хранения порядка обхода,
 *       поэтому не все методы могут быть constexpr.
 */
class FC2DArea
{
public:
    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Создаёт область 1×1 ячейку размером 1×1 с началом в (0, 0).
    FC2DArea() noexcept
        : _startPoint{0, 0}
        , _areaSize{1, 1}
        , _cellCounts{1, 1}
    {}

    /// @brief Конструктор с параметрами.
    /// @param startPoint Начальная точка области (левый верхний угол).
    /// @param areaSize   Размер всей области (по умолчанию 1×1).
    /// @param cellCounts Количество ячеек по осям (Nx, Ny). Значения < 1 автоматически ограничиваются до 1.
    /// @note Координаты и размеры используют тип float, количество ячеек — целочисленные значения,
    ///       хранящиеся во float-полях FC2DPoint для совместимости с геометрическими примитивами.
    FC2DArea(const FC2DPoint& startPoint, const FC2DSize& areaSize = {1, 1}, const FC2DPoint& cellCounts = {1, 1}) noexcept
        : _startPoint{startPoint}
        , _areaSize{areaSize}
        , _cellCounts{
            std::max(1.0f, cellCounts.x()),
            std::max(1.0f, cellCounts.y())
          }
    {}

    /// @brief Конструктор копирования.
    FC2DArea(const FC2DArea&) = default;

    /// @brief Оператор присваивания копированием.
    FC2DArea& operator=(const FC2DArea&) = default;

    /// @brief Деструктор.
    ~FC2DArea() = default;

    // --- геттеры ---
    /// @brief Возвращает полный размер области.
    /// @return Размер области (ширина × высота).
    [[nodiscard]] const FC2DSize& size() const noexcept { return _areaSize; }

    /// @brief Возвращает начальную точку области (левый верхний угол).
    /// @return Координаты начала области.
    [[nodiscard]] FC2DPoint startPoint() const noexcept { return _startPoint; }

    /// @brief Возвращает количество ячеек по осям (Nx, Ny).
    /// @return Точка, где x = количество ячеек по X, y = количество ячеек по Y.
    [[nodiscard]] FC2DPoint cellCounts() const noexcept { return _cellCounts; }

    /// @brief Возвращает количество ячеек по оси X (Nx).
    /// @return Целое число ≥ 1.
    [[nodiscard]] int cellCountX() const noexcept { return static_cast<int>(_cellCounts.x()); }

    /// @brief Возвращает количество ячеек по оси Y (Ny).
    /// @return Целое число ≥ 1.
    [[nodiscard]] int cellCountY() const noexcept { return static_cast<int>(_cellCounts.y()); }

    /// @brief Возвращает общее количество ячеек в области (Nx × Ny).
    /// @return Произведение количества ячеек по обеим осям.
    [[nodiscard]] int totalCells() const noexcept { return cellCountX() * cellCountY(); }

    /// @brief Вычисляет размер одной ячейки (равномерное деление области на сетку).
    /// @return Размер ячейки: (areaSize.width / Nx, areaSize.height / Ny).
    [[nodiscard]] FC2DSize cellSize() const noexcept
    {
        return FC2DSize{
            _areaSize.width()  / _cellCounts.x(),
            _areaSize.height() / _cellCounts.y()
        };
    }

    /// @brief Проверяет, является ли область пустой (нулевое или отрицательное количество ячеек).
    /// @return true, если общее количество ячеек ≤ 0; иначе — false.
    [[nodiscard]] bool isEmpty() const noexcept { return totalCells() <= 0; }

    // --- управление порядком обхода ячеек ---
    /// @brief Устанавливает кастомный порядок обхода ячеек по линейным индексам.
    /// @param order Вектор линейных индексов ячеек (idx = i + j * Nx) в желаемом порядке обхода.
    /// @note Если порядок не задан (пустой вектор), используется последовательный обход 0, 1, 2, ..., N-1.
    void setTraversalOrder(const QVector<int>& order)
    {
        _traversalOrder = order;
        _traversalIndex = 0;
    }

    /// @brief Сбрасывает текущую позицию обхода на начало.
    void resetTraversal() { _traversalIndex = 0; }

    /// @brief Возвращает размер текущего порядка обхода.
    /// @return Количество элементов в кастомном порядке, либо totalCells(), если порядок не задан.
    [[nodiscard]] int traversalSize() const
    {
        return _traversalOrder.isEmpty() ? totalCells() : _traversalOrder.size();
    }

    /// @brief Возвращает следующую ячейку в порядке обхода и продвигает индекс.
    /// @return Ячейка по текущему индексу обхода, либо FC2DCellError при выходе за границы.
    /// @note При первом вызове возвращает первую ячейку в порядке, при последующих — следующие.
    ///       После завершения обхода всегда возвращает FC2DCellError.
    [[nodiscard]] FC2DCell nextInTraversal()
    {
        if (_traversalOrder.isEmpty())
        {
            if (_traversalIndex < 0 || _traversalIndex >= totalCells())
            {
                return FC2DCellError;
            }
            FC2DCell cell = atLinear(_traversalIndex);
            ++_traversalIndex;
            return cell;
        }
        else
        {
            if (_traversalIndex < 0 || _traversalIndex >= _traversalOrder.size())
            {
                return FC2DCellError;
            }
            int idx = _traversalOrder[_traversalIndex];
            ++_traversalIndex;
            return atLinear(idx);
        }
    }

    /// @brief Возвращает текущую позицию в порядке обхода.
    /// @return Индекс следующей ячейки, которая будет возвращена методом nextInTraversal().
    [[nodiscard]] int currentTraversalIndex() const { return _traversalIndex; }

    /// @brief Проверяет, завершён ли обход всех ячеек.
    /// @return true, если достигнут конец порядка обхода; иначе — false.
    [[nodiscard]] bool isTraversalFinished() const
    {
        return _traversalOrder.isEmpty()
            ? _traversalIndex >= totalCells()
            : _traversalIndex >= _traversalOrder.size();
    }

    // --- доступ к ячейкам ---
    /// @brief Возвращает ячейку по линейному индексу (row-major порядок: idx = i + j * Nx).
    /// @param linearIndex Линейный индекс ячейки в диапазоне [0, totalCells() - 1].
    /// @return Ячейка с вычисленной позицией и размером, либо FC2DCellError при некорректном индексе.
    [[nodiscard]] FC2DCell atLinear(int linearIndex) const noexcept
    {
        if (linearIndex < 0 || linearIndex >= totalCells())
        {
            return FC2DCellError;
        }
        int Nx = cellCountX();
        int j = linearIndex / Nx;
        int i = linearIndex % Nx;
        return at(i, j);
    }

    /// @brief Возвращает ячейку по двумерным индексам (i, j).
    /// @param i Индекс ячейки по оси X (столбец), диапазон [0, Nx - 1].
    /// @param j Индекс ячейки по оси Y (строка), диапазон [0, Ny - 1].
    /// @return Ячейка с вычисленной позицией и размером, либо FC2DCellError при выходе за границы сетки.
    [[nodiscard]] FC2DCell at(int i, int j) const noexcept
    {
        if (i < 0 || i >= cellCountX() || j < 0 || j >= cellCountY())
        {
            return FC2DCellError;
        }

        FC2DSize cs = cellSize();
        FC2DPoint anchor{
            _startPoint.x() + static_cast<float>(i) * cs.width(),
            _startPoint.y() + static_cast<float>(j) * cs.height()
        };

        return FC2DCell{anchor, cs};
    }

    /// @brief Возвращает ячейку, содержащую заданную точку.
    /// @param point Координаты точки в глобальной системе.
    /// @return Ячейка, содержащая точку (включая левую и верхнюю границы), либо FC2DCellError,
    ///         если точка находится вне области.
    [[nodiscard]] FC2DCell at(const FC2DPoint& point) const noexcept
    {
        if (!contains(point))
        {
            return FC2DCellError;
        }
        FC2DSize cs = cellSize();
        int i = static_cast<int>(std::floor((point.x() - _startPoint.x()) / cs.width()));
        int j = static_cast<int>(std::floor((point.y() - _startPoint.y()) / cs.height()));
        // Ограничение индексов на случай погрешностей округления на правой/нижней границе
        i = std::min(i, cellCountX() - 1);
        j = std::min(j, cellCountY() - 1);
        return at(i, j);
    }

    // --- вспомогательные методы ---
    /// @brief Вычисляет индексы ячейки (i, j), содержащей заданную точку.
    /// @param point Координаты точки в глобальной системе.
    /// @return Точка с индексами (i, j), либо FC2DPointError, если точка вне области.
    [[nodiscard]] FC2DPoint indexAt(const FC2DPoint& point) const noexcept
    {
        if (!contains(point))
        {
            return FC2DPointError;
        }
        FC2DSize cs = cellSize();
        int i = static_cast<int>(std::floor((point.x() - _startPoint.x()) / cs.width()));
        int j = static_cast<int>(std::floor((point.y() - _startPoint.y()) / cs.height()));
        // Ограничение индексов на случай погрешностей округления на правой/нижней границе
        i = std::min(i, cellCountX() - 1);
        j = std::min(j, cellCountY() - 1);
        return FC2DPoint{static_cast<float>(i), static_cast<float>(j)};
    }

    /// @brief Проверяет, принадлежит ли точка области (включая границы).
    /// @param point Проверяемая точка.
    /// @return true, если точка находится внутри или на границе области; иначе — false.
    /// @note Точка с координатами равными правой или нижней границе считается принадлежащей области.
    [[nodiscard]] constexpr bool contains(const FC2DPoint& point) const noexcept
    {
        if (point == FC2DPointError)
        {
            return false;
        }
        return point.x() >= _startPoint.x() && point.x() <= _startPoint.x() + _areaSize.width() &&
               point.y() >= _startPoint.y() && point.y() <= _startPoint.y() + _areaSize.height();
    }

    // --- операторы сравнения ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другая область для сравнения.
    /// @return true, если startPoint, areaSize и cellCounts идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FC2DArea& other) const noexcept
    {
        return _startPoint == other._startPoint &&
               _areaSize == other._areaSize &&
               _cellCounts == other._cellCounts;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другая область для сравнения.
    /// @return true, если области различаются хотя бы по одному параметру; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FC2DArea& other) const noexcept
    {
        return !(*this == other);
    }

private:
    /// Начальная точка области (левый верхний угол).
    FC2DPoint _startPoint;

    /// Полный размер области (ширина × высота).
    FC2DSize _areaSize;

    /// Количество ячеек по осям (Nx, Ny), хранится как точка для совместимости с геометрическими операциями.
    /// Значения всегда ≥ 1.0.
    FC2DPoint _cellCounts;

    /// Кастомный порядок обхода ячеек по линейным индексам (опционально).
    QVector<int> _traversalOrder;

    /// Текущая позиция в порядке обхода.
    int _traversalIndex = 0;
};

// --- внешние псевдонимы типов ---
/// Список областей (значения), используемый для хранения коллекций 2D-областей.
using FC2DAreaList = QList<FC2DArea>;

/// Список указателей на области, применяется при работе с полиморфными объектами.
using FC2DPtrAreaList = QList<FC2DArea*>;

/// @brief Возвращает предопределённую "ошибочную" область с недопустимыми параметрами.
/// @return Область с startPoint=(-1,-1), areaSize=(-1,-1), cellCounts=(-1,-1).
/// @note Реализовано как функция (а не constexpr-переменная), так как класс содержит
///       нетривиальные члены (QVector), что запрещает статическую инициализацию constexpr.
[[nodiscard]] inline FC2DArea FC2DAreaError() noexcept
{
    static const FC2DArea error{FC2DPoint{-1, -1}, FC2DSize{-1, -1}, FC2DPoint{-1, -1}};
    return error;
}

#endif // FC_2D_AREA_H
