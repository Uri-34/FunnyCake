#ifndef FC_SVG_FIGURE_H
#define FC_SVG_FIGURE_H

#include <QList>
#include <QString>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QRectF>
#include <QPointF>

#include "FC2DPoint.h"

/**
 * @brief Класс для представления фигуры в SVG (имя + стиль + путь).
 *
 * Хранит векторную фигуру с именем, стилем отрисовки (QPen/QBrush) и
 * геометрическим путём (QPainterPath). Используется при парсинге SVG
 * и генерации G-кода для плоттера.
 *
 * Ключевые возможности:
 *   - Хранение имени фигуры (для идентификации в слоях Inkscape),
 *   - Стиль отрисовки (цвет, толщина линии, тип кисти),
 *   - Геометрический путь (последовательность точек и кривых),
 *   - Проверка на пустоту и валидность,
 *   - Получение ограничивающего прямоугольника,
 *   - Доступ к точкам пути для генерации G-кода,
 *   - Трансформации (перемещение, масштабирование).
 *
 * @note Класс тривиально копируемый и перемещаемый — безопасен для использования
 *       в контейнерах Qt (QList, QVector) и передачи по значению.
 * @see FCSVGLayer для группировки фигур в слои
 * @see FCSVGImageContainer для хранения полной модели
 */
class FCSVGFigure
{
public:
    // Конструкторы
    /**
     * @brief Конструктор с полной инициализацией всех полей.
     * @param name Имя фигуры (например, из атрибута id в SVG).
     * @param pen Перо для отрисовки контура (цвет, толщина, стиль).
     * @param brush Кисть для заполнения области (цвет, градиент, узор).
     * @param path Геометрический путь фигуры (последовательность точек).
     * @note Все параметры имеют значения по умолчанию для создания пустой фигуры.
     */
    explicit FCSVGFigure(const QString &name = QString(), QPen pen = QPen(), QBrush brush = QBrush(), QPainterPath path = QPainterPath()) noexcept
        : _name(name),
          _pen(pen),
          _brush(brush),
          _path(path)
    {}

    /**
     * @brief Конструктор копирования (генерируется компилятором).
     * @note Тривиальная копия всех полей.
     */
    FCSVGFigure(const FCSVGFigure &) = default;

    /**
     * @brief Конструктор перемещения (генерируется компилятором).
     * @note Эффективное перемещение без копирования данных.
     */
    FCSVGFigure(FCSVGFigure &&) noexcept = default;

    /**
     * @brief Деструктор (генерируется компилятором).
     * @note Тривиальное освобождение ресурсов.
     */
    ~FCSVGFigure() = default;

    // Операторы присваивания
    /**
     * @brief Оператор присваивания копированием.
     * @return Ссылка на текущий объект.
     */
    FCSVGFigure &operator=(const FCSVGFigure &) = default;

    /**
     * @brief Оператор присваивания перемещением.
     * @return Ссылка на текущий объект.
     */
    FCSVGFigure &operator=(FCSVGFigure &&) noexcept = default;

    // Геттеры (только чтение)
    /**
     * @brief Возвращает имя фигуры.
     * @return Константная ссылка на имя (из атрибута id в SVG).
     */
    [[nodiscard]] inline const QString &name() const noexcept { return _name; }

    /**
     * @brief Возвращает перо для отрисовки контура.
     * @return Константная ссылка на QPen.
     */
    [[nodiscard]] inline const QPen &pen() const noexcept { return _pen; }

    /**
     * @brief Возвращает кисть для заполнения области.
     * @return Константная ссылка на QBrush.
     */
    [[nodiscard]] inline const QBrush &brush() const noexcept { return _brush; }

    /**
     * @brief Возвращает геометрический путь фигуры.
     * @return Константная ссылка на QPainterPath.
     */
    [[nodiscard]] inline const QPainterPath &path() const noexcept { return _path; }

    /**
     * @brief Проверяет, пуст ли путь фигуры.
     * @return true если путь не содержит элементов; иначе — false.
     */
    [[nodiscard]] inline bool isEmpty() noexcept { return _path.isEmpty(); }

    /**
     * @brief Возвращает количество элементов в пути.
     * @return Число точек и кривых в QPainterPath.
     */
    [[nodiscard]] inline int count() noexcept { return _path.elementCount(); }

    /**
     * @brief Возвращает ограничивающий прямоугольник фигуры.
     * @return QRectF с координатами границ (x, y, width, height).
     * @note Вычисляется динамически через path.boundingRect().
     */
    [[nodiscard]] inline QRectF boundingRect() const noexcept { return _path.boundingRect(); }

    /**
     * @brief Возвращает точку пути по индексу.
     * @param index Индекс элемента пути (0-based).
     * @return FC2DPoint с координатами точки.
     * @warning Если индекс выходит за границы — возвращается точка (0, 0).
     */
    [[nodiscard]] FC2DPoint pointAt(int index) const noexcept
    {
        if (index < 0 || index >= _path.elementCount())
        {
            return FC2DPoint(0.0, 0.0);
        }
        const QPainterPath::Element &elem = _path.elementAt(index);
        return FC2DPoint(static_cast<qreal>(elem.x), static_cast<qreal>(elem.y));
    }

    /**
     * @brief Возвращает первую точку пути (начало фигуры).
     * @return FC2DPoint с координатами первой точки.
     * @note Если путь пуст — возвращается точка (0, 0).
     */
    [[nodiscard]] inline FC2DPoint startPoint() const noexcept { return pointAt(0); }

    /**
     * @brief Возвращает последнюю точку пути (конец фигуры).
     * @return FC2DPoint с координатами последней точки.
     * @note Если путь пуст — возвращается точка (0, 0).
     */
    [[nodiscard]] inline FC2DPoint endPoint() const noexcept { return pointAt(_path.elementCount() - 1); }

    // Сеттеры (модификация)
    /**
     * @brief Устанавливает имя фигуры.
     * @param name Новое имя (из атрибута id в SVG).
     */
    inline void setName(const QString &name) noexcept { _name = name; }

    /**
     * @brief Устанавливает перо для отрисовки контура.
     * @param pen Новое значение QPen (цвет, толщина, стиль).
     */
    inline void setPen(const QPen &pen) noexcept { _pen = pen; }

    /**
     * @brief Устанавливает кисть для заполнения области.
     * @param brush Новое значение QBrush (цвет, градиент, узор).
     */
    inline void setBrush(const QBrush &brush) noexcept { _brush = brush; }

    /**
     * @brief Устанавливает геометрический путь фигуры.
     * @param path Новый QPainterPath (последовательность точек).
     */
    inline void setPath(const QPainterPath &path) noexcept { _path = path; }

    /**
     * @brief Добавляет путь к текущему пути фигуры.
     * @param other Путь для добавления (объединение контуров).
     */
    inline void addPath(const QPainterPath &other) { _path.addPath(other); }

    /**
     * @brief Перемещает путь на заданный вектор (dx, dy).
     * @param dx Смещение по оси X (мм).
     * @param dy Смещение по оси Y (мм).
     */
    inline void translate(qreal dx, qreal dy) { _path.translate(dx, dy); }

    /**
     * @brief Очищает путь фигуры (делает фигуру пустой).
     * @note Имя и стиль сохраняются, очищается только геометрия.
     */
    inline void clearPath() noexcept { _path = QPainterPath(); }

    /**
     * @brief Полностью очищает фигуру (имя, стиль, путь).
     * @note Восстанавливает значения по умолчанию для всех полей.
     */
    void clear() noexcept
    {
        _name.clear();
        _pen = QPen();
        _brush = QBrush();
        _path = QPainterPath();
    }

    // Валидация
    /**
     * @brief Проверяет валидность фигуры для генерации G-кода.
     * @return true если путь не пуст и содержит хотя бы 2 точки; иначе — false.
     */
    [[nodiscard]] inline bool isValid() const noexcept { return !_path.isEmpty() && _path.elementCount() >= 2; }

    // Операторы сравнения
    /**
     * @brief Оператор сравнения на равенство.
     * @param other Другая фигура для сравнения.
     * @return true если все поля идентичны; иначе — false.
     * @note Сравниваются имя, перо, кисть и количество элементов пути.
     */
    [[nodiscard]] inline bool operator==(const FCSVGFigure &other) const noexcept
    {
        return _name == other._name && _pen == other._pen && _brush == other._brush && _path.elementCount() == other._path.elementCount();
    }

    /**
     * @brief Оператор сравнения на неравенство.
     * @param other Другая фигура для сравнения.
     * @return true если фигуры различаются хотя бы по одному полю; иначе — false.
     */
    [[nodiscard]] inline bool operator!=(const FCSVGFigure &other) const noexcept { return !(*this == other); }

private:
    /// Имя фигуры (из атрибута id в SVG).
    QString _name;

    /// Перо для отрисовки контура (цвет, толщина, стиль).
    QPen _pen;

    /// Кисть для заполнения области (цвет, градиент, узор).
    QBrush _brush;

    /// Геометрический путь (последовательность точек и кривых).
    QPainterPath _path;
};

/// Псевдоним для списка фигур.
using FCSVGFigureList = QList<FCSVGFigure>;

// Поддержка Qt Meta-Object System (опционально)

// Q_DECLARE_METATYPE(FCSVGFigure)
// Q_DECLARE_METATYPE(FCSVGFigureList)

#endif // FC_SVG_FIGURE_H
