#ifndef FC_SVG_LAYER_H
#define FC_SVG_LAYER_H

#include <QList>
#include <QString>

#include "FCConditionObject.h"
#include "FCSVGFigure.h"

/**
 * @brief Класс для представления одного слоя SVG (Z-уровня).
 *
 * Хранит коллекцию фигур (FCSVGFigure), принадлежащих одному слою
 * Inkscape, а также метаданные слоя (номер, толщина, Z-позиция).
 *
 * @note Наследует FCConditionObject → QObject, поэтому:
 *       - Копирование запрещено (QObject не копируется),
 *       - Перемещение запрещено (QObject не перемещается),
 *       - Передавать только по указателю или ссылке.
 * @see FCSVGFigure для представления отдельных фигур
 * @see FCSVGImageContainer для хранения полной модели
 */
class FCSVGLayer
    : public FCConditionObject
{
Q_OBJECT
public:
    // Конструкторы / Деструктор
    /**
     * @brief Конструктор слоя с именем и толщиной.
     * @param name Имя слоя (из атрибута label в Inkscape).
     * @param thickness Толщина слоя в мм (для Z-координаты плоттера).
     * @param parent Родительский QObject для управления временем жизни.
     * @note Начальное состояние: ReadyState::NotReady, PlayState::Stop.
     */
    explicit FCSVGLayer(const QString &name = QString(), qreal thickness = 1.0, QObject *parent = nullptr)
        : FCConditionObject(name, parent),
          _thickness(thickness),
          _zPosition(0.0)
    {}

    /**
     * @brief Деструктор (генерируется компилятором).
     * @note QObject автоматически удалит дочерние объекты.
     */
    ~FCSVGLayer() = default;

    // Запрет копирования и перемещения (QObject не поддерживает)
    /**
     * @brief Конструктор копирования — удалён (QObject не копируется).
     */
    FCSVGLayer(const FCSVGLayer &) = delete;

    /**
     * @brief Оператор присваивания копированием — удалён.
     */
    FCSVGLayer &operator=(const FCSVGLayer &) = delete;

    /**
     * @brief Конструктор перемещения — удалён (QObject не перемещается).
     */
    FCSVGLayer(FCSVGLayer &&) = delete;

    /**
     * @brief Оператор присваивания перемещением — удалён.
     */
    FCSVGLayer &operator=(FCSVGLayer &&) = delete;

    // Геттеры (только чтение)
    /**
     * @brief Возвращает толщину слоя.
     * @return Толщина в мм (для Z-координаты плоттера).
     */
    [[nodiscard]] inline qreal thickness() const noexcept { return _thickness; }

    /**
     * @brief Возвращает Z-позицию слоя.
     * @return Координата Z в мм (для G-кода).
     */
    [[nodiscard]] inline qreal zPosition() const noexcept { return _zPosition; }

    /**
     * @brief Возвращает список всех фигур в слое.
     * @return Константная ссылка на QList<FCSVGFigure>.
     */
    [[nodiscard]] inline const FCSVGFigureList &figures() const noexcept { return _figures; }

    /**
     * @brief Возвращает список всех фигур в слое (для модификации).
     * @return Ссылка на QList<FCSVGFigure>.
     */
    [[nodiscard]] inline FCSVGFigureList &figures() noexcept { return _figures; }

    /**
     * @brief Возвращает количество фигур в слое.
     * @return Число фигур в списке.
     */
    [[nodiscard]] inline int count() const noexcept { return _figures.size(); }

    /**
     * @brief Возвращает фигуру по индексу.
     * @param index Индекс фигуры (0-based).
     * @return Константная ссылка на FCSVGFigure.
     * @warning Если индекс выходит за границы — поведение не определено.
     */
    [[nodiscard]] inline const FCSVGFigure &figureAt(int index) const noexcept { return _figures.at(index); }

    /**
     * @brief Находит список фигур по имени.
     * @param name Имя для поиска (из атрибута id в SVG).
     * @return Список всех совпадений (может быть пустым).
     */
    [[nodiscard]] FCSVGFigureList figuresByName(const QString &name) const
    {
        FCSVGFigureList result;
        for (const auto &figure : _figures)
        {
            if (figure.name() == name)
            {
                result.append(figure);
            }
        }
        return result;
    }

    // Сеттеры (модификация)
    /**
     * @brief Устанавливает толщину слоя.
     * @param thickness Толщина в мм (для Z-координаты плоттера).
     */
    inline void setThickness(qreal thickness) noexcept { _thickness = thickness; }

    /**
     * @brief Устанавливает Z-позицию слоя.
     * @param zPosition Координата Z в мм (для G-кода).
     */
    inline void setZPosition(qreal zPosition) noexcept { _zPosition = zPosition; }

    /**
     * @brief Добавляет фигуру в слой (копированием).
     * @param figure Фигура для добавления.
     */
    inline void append(const FCSVGFigure &figure) { _figures.append(figure); }

    /**
     * @brief Добавляет фигуру в слой (перемещением).
     * @param figure Фигура для добавления.
     */
    inline void append(FCSVGFigure &&figure) { _figures.append(std::move(figure)); }

    /**
     * @brief Удаляет фигуру по индексу.
     * @param index Индекс фигуры для удаления (0-based).
     * @return true если фигура удалена; иначе — false.
     */
    bool removeFigureAt(int index)
    {
        if (index < 0 || index >= _figures.size())
        {
            return false;
        }
        _figures.removeAt(index);
        return true;
    }

    /**
     * @brief Очищает список фигур слоя.
     * @note Метаданные (толщина, Z-позиция) сохраняются.
     */
    inline void clear() noexcept { _figures.clear(); }

    // Валидация
    /**
     * @brief Проверяет, готов ли слой к генерации G-кода.
     * @return true если слой содержит хотя бы одну валидную фигуру.
     */
    [[nodiscard]] bool isReady() const noexcept
    {
        if (_figures.isEmpty())
        {
            return false;
        }
        for (const auto &figure : _figures)
        {
            if (figure.isValid())
            {
                return true;
            }
        }
        return false;
    }

    // Операторы сравнения
    /**
     * @brief Оператор сравнения на равенство.
     * @param other Другой слой для сравнения.
     * @return true если все поля идентичны.
     */
    [[nodiscard]] bool operator==(const FCSVGLayer &other) const noexcept
    {
        return objectName() == other.objectName() && _thickness == other._thickness && _zPosition == other._zPosition && _figures == other._figures;
    }

    /**
     * @brief Оператор сравнения на неравенство.
     * @param other Другой слой для сравнения.
     * @return true если слои различаются.
     */
    [[nodiscard]] inline bool operator!=(const FCSVGLayer &other) const noexcept { return !(*this == other); }

private:
    /// Толщина слоя в мм (для Z-координаты плоттера).
    qreal _thickness = 1.0;

    /// Z-позиция слоя в мм (cumulative thickness для G-кода).
    qreal _zPosition = 0.0;

    /// Список фигур, принадлежащих этому слою.
    FCSVGFigureList _figures;
};

/// Псевдоним для списка указателей на слои (QObject не копируется!).
using FCSVGLayerList = QList<FCSVGLayer*>;

#endif // FC_SVG_LAYER_H
