#ifndef FC_SPEED_H
#define FC_SPEED_H

#include <QList>
#include <cstdint>
#include <type_traits>

/**
 * @brief Класс для хранения пары целочисленных скоростей движения и отрисовки.
 *
 * Предназначен для компактного представления двух независимых скоростей в единой структуре:
 *   - Скорость движения (_move) — линейная/угловая скорость исполнительного механизма
 *     (мм/с, шаги/с, об/мин в зависимости от контекста),
 *   - Скорость отрисовки (_draw) — частота обновления графического вывода или кадров
 *     (кадры/с, точки/с).
 *
 * Особенности реализации:
 *   - Использует 32-битные знаковые целые числа (int32_t) для предсказуемого размера
 *     на всех платформах (ровно 8 байт),
 *   - Поддерживает отрицательные значения для реверса или движения назад,
 *   - Тривиально копируемый (trivially copyable) для эффективного использования
 *     в контейнерах и встраиваемых системах,
 *   - Все операции помечены как constexpr и noexcept для максимальной эффективности.
 *
 * @note Семантика полей зависит от контекста использования:
 *       - В робототехнике: _move = скорость колёс/привода, _draw = скорость обновления дисплея,
 *       - В графике: _move = скорость анимации объекта, _draw = частота кадров,
 *       - В ЧПУ: _move = подача инструмента, _draw = скорость прорисовки траектории.
 * @warning Класс НЕ выполняет автоматическую проверку допустимых диапазонов скоростей.
 *          Для ограничения значений используйте метод clamped() или наследуйте класс
 *          с дополнительной валидацией.
 */
class FCSpeed
{
public:
    /// Тип данных для хранения значений скорости (32-битное знаковое целое).
    /// Использование знакового типа позволяет представлять реверс/движение назад.
    using _type = int32_t;

    // --- константы ---
    /// @brief Возвращает нулевую скорость (движение = 0, отрисовка = 0).
    /// @return Константный объект скорости с нулевыми значениями.
    [[nodiscard]] static constexpr FCSpeed zero() noexcept { return FCSpeed{0, 0}; }

    // --- конструкторы ---
    /// @brief Конструктор по умолчанию. Инициализирует обе скорости нулевыми значениями.
    constexpr FCSpeed() noexcept = default;

    /// @brief Конструктор с параметрами.
    /// @param move Скорость движения (линейная/угловая скорость механизма).
    /// @param draw Скорость отрисовки (частота обновления графического вывода).
    /// @note Отрицательные значения допустимы и интерпретируются как реверс/движение назад.
    constexpr FCSpeed(_type move, _type draw) noexcept
        : _move(move)
        , _draw(draw)
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FCSpeed(const FCSpeed&) noexcept = default;

    /// @brief Оператор присваивания копированием (тривиальный).
    constexpr FCSpeed& operator=(const FCSpeed&) noexcept = default;

    /// @brief Деструктор (тривиальный).
    ~FCSpeed() = default;

    // --- геттеры ---
    /// @brief Возвращает скорость движения механизма.
    /// @return Значение скорости движения (знаковое целое).
    /// @note Отрицательное значение означает движение в обратном направлении (реверс).
    [[nodiscard]] constexpr _type move() const noexcept { return _move; }

    /// @brief Возвращает скорость отрисовки/обновления.
    /// @return Значение скорости отрисовки (знаковое целое).
    /// @note В контексте графики обычно положительное значение (частота кадров).
    [[nodiscard]] constexpr _type draw() const noexcept { return _draw; }

    // --- сеттеры ---
    /// @brief Устанавливает новое значение скорости движения.
    /// @param v Новое значение скорости движения.
    /// @note Отрицательные значения допустимы для реверса.
    constexpr void setMove(_type v) noexcept { _move = v; }

    /// @brief Устанавливает новое значение скорости отрисовки.
    /// @param v Новое значение скорости отрисовки.
    /// @note В большинстве случаев следует использовать положительные значения.
    constexpr void setDraw(_type v) noexcept { _draw = v; }

    /// @brief Устанавливает обе скорости одновременно.
    /// @param move Новое значение скорости движения.
    /// @param draw Новое значение скорости отрисовки.
    constexpr void set(_type move, _type draw) noexcept
    {
        _move = move;
        _draw = draw;
    }

    // --- проверки состояния ---
    /// @brief Проверяет, равны ли обе скорости нулю.
    /// @return true, если _move == 0 и _draw == 0; иначе — false.
    /// @note Эквивалентно сравнению с FCSpeed::zero().
    [[nodiscard]] constexpr bool isZero() const noexcept { return _move == 0 && _draw == 0; }

    /// @brief Проверяет, является ли скорость допустимой (не выходит за пределы разумных значений).
    /// @return true, если обе компоненты находятся в диапазоне [-1'000'000, 1'000'000]; иначе — false.
    /// @note Диапазон выбран как разумный предел для большинства встраиваемых приложений.
    ///       Для строгой валидации используйте внешние проверки или наследование.
    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        constexpr _type limit = 1'000'000;
        return _move >= -limit && _move <= limit &&
               _draw >= -limit && _draw <= limit;
    }

    // --- операторы сравнения ---
    /// @brief Оператор сравнения на равенство.
    /// @param other Другой объект скорости для сравнения.
    /// @return true, если обе компоненты (_move и _draw) идентичны; иначе — false.
    [[nodiscard]] constexpr bool operator==(const FCSpeed& other) const noexcept
    {
        return _move == other._move && _draw == other._draw;
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param other Другой объект скорости для сравнения.
    /// @return true, если скорости различаются хотя бы по одной компоненте; иначе — false.
    [[nodiscard]] constexpr bool operator!=(const FCSpeed& other) const noexcept
    {
        return !(*this == other);
    }

    // --- арифметические операторы ---
    /// @brief Оператор составного сложения.
    /// @param other Добавляемая скорость.
    /// @return Ссылка на текущий объект после модификации.
    /// @note Компоненты складываются независимо: _move += other._move, _draw += other._draw.
    constexpr FCSpeed& operator+=(const FCSpeed& other) noexcept
    {
        _move += other._move;
        _draw += other._draw;
        return *this;
    }

    /// @brief Оператор составного вычитания.
    /// @param other Вычитаемая скорость.
    /// @return Ссылка на текущий объект после модификации.
    /// @note Компоненты вычитаются независимо: _move -= other._move, _draw -= other._draw.
    constexpr FCSpeed& operator-=(const FCSpeed& other) noexcept
    {
        _move -= other._move;
        _draw -= other._draw;
        return *this;
    }

    /// @brief Оператор сложения.
    /// @param other Добавляемая скорость.
    /// @return Новый объект скорости с суммой соответствующих компонент.
    [[nodiscard]] constexpr FCSpeed operator+(const FCSpeed& other) const noexcept
    {
        return FCSpeed{_move + other._move, _draw + other._draw};
    }

    /// @brief Оператор вычитания.
    /// @param other Вычитаемая скорость.
    /// @return Новый объект скорости с разностью соответствующих компонент.
    [[nodiscard]] constexpr FCSpeed operator-(const FCSpeed& other) const noexcept
    {
        return FCSpeed{_move - other._move, _draw - other._draw};
    }

    /// @brief Оператор унарного минуса (инверсия направления).
    /// @return Новый объект скорости с инвертированными компонентами (-_move, -_draw).
    /// @note Полезен для быстрого переключения направления движения.
    [[nodiscard]] constexpr FCSpeed operator-() const noexcept
    {
        return FCSpeed{-_move, -_draw};
    }

    // --- вспомогательные методы ---
    /// @brief Ограничивает обе компоненты скорости заданным диапазоном.
    /// @param min Минимальное допустимое значение.
    /// @param max Максимальное допустимое значение.
    /// @return Новый объект скорости с компонентами, ограниченными диапазоном [min, max].
    /// @note Применяется один и тот же диапазон к обеим компонентам. Для раздельного
    ///       ограничения используйте отдельные вызовы с последующей сборкой.
    [[nodiscard]] constexpr FCSpeed clamped(_type min, _type max) const noexcept
    {
        auto clamp = [](_type v, _type lo, _type hi) constexpr -> _type {
            return (v < lo) ? lo : ((v > hi) ? hi : v);
        };
        return FCSpeed{
            clamp(_move, min, max),
            clamp(_draw, min, max)
        };
    }

    /// @brief Масштабирует обе компоненты скорости на заданный коэффициент.
    /// @param factor Целочисленный коэффициент масштабирования.
    /// @return Новый объект скорости с умноженными на фактор компонентами.
    /// @note Для дробного масштабирования используйте внешнее преобразование типов.
    [[nodiscard]] constexpr FCSpeed scaled(_type factor) const noexcept
    {
        return FCSpeed{_move * factor, _draw * factor};
    }

private:
    /// Скорость движения механизма (линейная/угловая). Отрицательное значение = реверс.
    _type _move = 0;

    /// Скорость отрисовки/обновления графического вывода (частота кадров, точки/с).
    _type _draw = 0;
};

// --- псевдонимы типов ---
/// Список скоростей (значения), используемый для хранения коллекций объектов скорости.
template<typename T = FCSpeed>
using FCSpeedList = QList<T>;

// --- поддержка системы мета-объектов Qt (опционально) ---
// Раскомментируйте при необходимости использования в QVariant, моделях и сигналах/слотах:
/*
#include <QMetaType>
#include <QDebug>

Q_DECLARE_METATYPE(FCSpeed)

inline QDebug operator<<(QDebug dbg, const FCSpeed& speed)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "FCSpeed(move=" << speed.move()
                  << ", draw=" << speed.draw() << ")";
    return dbg;
}
*/

// --- проверки компиляции ---
// Гарантирует тривиальную копируемость для использования в встраиваемых системах
static_assert(std::is_trivially_copyable_v<FCSpeed>, "FCSpeed должен быть тривиально копируемым типом");

// Гарантирует стандартную компоновку для совместимости с бинарными протоколами и внешними библиотеками
static_assert(std::is_standard_layout_v<FCSpeed>, "FCSpeed должен иметь стандартную компоновку (standard layout)");

// Гарантирует предсказуемый размер в памяти (2 × int32_t = 8 байт) для эффективного использования в массивах
static_assert(sizeof(FCSpeed) == 8, "Размер FCSpeed должен быть ровно 8 байт (2 × int32_t)");

// Проверка корректности константы нулевой скорости
static_assert(FCSpeed::zero().isZero(), "FCSpeed::zero() должен возвращать нулевую скорость");

#endif // FC_SPEED_H
