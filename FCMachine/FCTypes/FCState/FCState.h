#ifndef FC_STATE_H
#define FC_STATE_H

/**
 * @file FCState.h
 * @brief Централизованное управление состояниями системы Funny Cake Plotter
 *
 * Этот заголовочный файл определяет:
 * - Набор enum-классов для типизированных состояний системы
 * - Шаблонный класс FCStateT<> для хранения и управления состояниями
 * - Type traits для проверки валидности типов состояний
 * - Защиту от использования UI-типов в бизнес-логике
 *
 * @note Все enum имеют префикс "FC" для предотвращения конфликтов имён
 * @note Это шаблонный value-класс без сигналов.
 *       Сигналы должны эмититься в классах-владельцах (FCDisplay, FCMachine).
 * @note Не наследуется от QObject — не требует moc-обработки.
 * @note Требует C++17 для fold expressions и if constexpr.
 * @author K-Service
 * @version 2.1.0
 * @date 2026-03-19
 *
 * @warning КРИТИЧЕСКИ ВАЖНО:
 *          - FCErrorState — ТОЛЬКО для UI (FCDisplay), НЕ использовать в устройствах!
 *          - FCErrorType — для бизнес-логики (устройства, контроллеры, парсеры)
 *          - Нарушение этого правила приведёт к ошибке компиляции (static_assert)
 *
 * @example
 * @code
 * // ✅ ПРАВИЛЬНО: устройство использует только логические типы
 * using FCDeviceState = FCStateT<FCReadyState, FCErrorType>;
 *
 * // ✅ ПРАВИЛЬНО: UI может использовать оба типа
 * using FCDisplayState = FCStateT<FCReadyState, FCPlayState, FCChangedState,
 *                                  FCErrorState, FCErrorType, FCPanelState>;
 *
 * // ❌ НЕПРАВИЛЬНО: устройство не должно знать о критичности для UI
 * using BadDeviceState = FCStateT<FCReadyState, FCErrorState, FCErrorType>;  // ОШИБКА!
 * @endcode
 */

// ============================================================================
// ЗАГОЛОВКИ
// ============================================================================

#include <tuple>
#include <type_traits>
#include <QString>      // Для QString и QStringLiteral в diffDetails()

// ============================================================================
// ОПРЕДЕЛЕНИЯ ENUM-КЛАССОВ ДЛЯ ТИПИЗИРОВАННЫХ СОСТОЯНИЙ
// ============================================================================

/**
 * @enum FCReadyState
 * @brief Состояния готовности системы к работе
 * @details Определяет, готово ли устройство/модуль к выполнению команд.
 */
enum class FCReadyState { NotReady, Ready };

/**
 * @enum FCPlayState
 * @brief Состояния воспроизведения модели плоттером
 * @details Отслеживает текущий фазовый режим выполнения задачи.
 */
enum class FCPlayState { Stop, Start, Pause };

/**
 * @enum FCChangedState
 * @brief Состояния изменения конфигурации или данных
 * @details Фиксирует факт модификации внутренних настроек или буферов данных.
 */
enum class FCChangedState { Unchanged, Changed };

/**
 * @enum FCErrorState
 * @brief Уровни критичности ошибок (ТОЛЬКО для UI-слоёв)
 *
 * Значения:
 * - None: ошибок нет (не отображать ничего)
 * - Warning: предупреждение (отобразить жёлтым, не блокировать)
 * - Critical: критическая ошибка (отобразить красным, заблокировать интерфейс)
 *
 * @warning НЕ ИСПОЛЬЗУЙТЕ в бизнес-логике устройств!
 * @see FCErrorType для типов ошибок в бизнес-логике
 */
enum class FCErrorState { None, Warning, Critical };

/**
 * @enum FCErrorType
 * @brief Типы ошибок системы (для бизнес-логики)
 * @note None — отсутствие ошибок (не NoError!)
 * @details Перечисляет конкретные категории сбоев на уровне протокола/оборудования.
 */
enum class FCErrorType
{
    None,               ///< Ошибок нет
    Open,               ///< Ошибка открытия устройства/файла
    Close,              ///< Ошибка закрытия
    Connection,         ///< Ошибка подключения
    Disconnection,      ///< Разрыв соединения
    Create,             ///< Ошибка создания ресурса
    Destroy,            ///< Ошибка уничтожения
    Read,               ///< Ошибка чтения
    Write,              ///< Ошибка записи
    OutOfRange,         ///< Выход за диапазон
    Configuration,      ///< Ошибка конфигурации
    Parse,              ///< Ошибка парсинга
    Timeout,            ///< Таймаут операции
    EmergencyStoped,    ///< Аварийная остановка

    Motion              ///< Ошибка перемещения
};

/**
 * @enum FCPanelState
 * @brief Состояния активной панели интерфейса
 * @details Определяет, какой экран в данный момент отображается пользователю.
 */
enum class FCPanelState { Main, Statistics, ScreenSaver };

/**
 * @enum FCVisibilityState
 * @brief Состояния видимости дополнительных элементов интерфейса
 * @details Управляет отображением оверлеев, попапов или служебных виджетов.
 */
enum class FCVisibilityState { Hide, Show };

// ============================================================================
// TYPE TRAITS: Проверка валидности и семантики типов состояний
// ============================================================================

/**
 * @struct is_fc_state_type
 * @brief Базовый трейт: по умолчанию тип не является состоянием FC
 * @details Специализации добавляются вручную для каждого enum-класса выше.
 */
template<typename T> struct is_fc_state_type : std::false_type {};

template<> struct is_fc_state_type<FCReadyState> : std::true_type {};
template<> struct is_fc_state_type<FCPlayState> : std::true_type {};
template<> struct is_fc_state_type<FCErrorType> : std::true_type {};
template<> struct is_fc_state_type<FCPanelState> : std::true_type {};
template<> struct is_fc_state_type<FCVisibilityState> : std::true_type {};

/**
 * @struct is_ui_only_state
 * @brief Маркер: тип состояния используется ТОЛЬКО в UI-слоях
 * @details Если true — тип не должен использоваться в бизнес-логике устройств.
 */
template<typename T> struct is_ui_only_state : std::false_type {};
template<> struct is_ui_only_state<FCPanelState> : std::true_type {};
template<> struct is_ui_only_state<FCVisibilityState> : std::true_type {};

/**
 * @struct is_logic_state
 * @brief Маркер: тип состояния используется в бизнес-логике
 * @details Если false — тип НЕ должен использоваться в устройствах/контроллерах.
 */
template<typename T> struct is_logic_state : std::true_type {};
template<> struct is_logic_state<FCPanelState> : std::false_type {};
template<> struct is_logic_state<FCVisibilityState> : std::false_type {};

// ============================================================================
// ШАБЛОННЫЙ КЛАСС: FCStateT<States...>
// ============================================================================

/**
 * @class FCStateT
 * @brief Шаблонный класс для управления состояниями с настраиваемым набором
 *
 * @tparam States... Вариативный список enum-классов состояний
 *
 * @note Не наследуется от QObject — сигналы эмитятся в классах-владельцах
 * @note Копируемый и перемещаемый (все члены — тривиальные типы)
 * @note Требует C++17 для fold expressions и if constexpr
 */
template<typename... States>
class FCStateT
{
    // Компиляционная проверка: все шаблонные параметры должны быть валидными состояниями FC
    static_assert((is_fc_state_type<States>::value && ...),
                  "All template parameters must be FC state enum types. "
                  "Did you forget to add: template<> struct is_fc_state_type<YourEnum> : std::true_type {};");

public:
    // ========================================================================
    // КОНСТРУКТОРЫ
    // ========================================================================

    /// @brief Конструктор по умолчанию
    /// @details Инициализирует все поля значениями по умолчанию (первый элемент enum).
    FCStateT() = default;

    /// @brief Вспомогательная функция: устанавливает значение в tuple по типу состояния
    /// @tparam State Тип состояния для установки
    /// @param values Ссылка на tuple значений
    /// @param value Новое значение
    /// @details Использует if constexpr для безопасного доступа по типу в tuple.
    template<typename State>
    static inline void setValueByType(std::tuple<States...>& values, State value)
    {
        if constexpr ((std::is_same_v<State, States> || ...)) {
            std::get<State>(values) = value;
        }
    }

    /// @brief Конструктор с инициализацией состояний по типу (ЛЮБОЙ ПОРЯДОК!)
    /// @tparam InitStates Типы инициализируемых состояний
    /// @param initialValues Значения для установки
    /// @details Автоматически распределяет значения по полям tuple на основе типов.
    template<typename... InitStates>
    explicit FCStateT(InitStates... initialValues)
    {
        static_assert((is_fc_state_type<InitStates>::value && ...),
                      "All constructor parameters must be FC state enum types");
        (setValueByType(_values, initialValues), ...);
    }

    // ========================================================================
    // ОПЕРАТОРЫ КОПИРОВАНИЯ И ПРИСВАИВАНИЯ
    // ========================================================================

    /// @brief Оператор копирующего присваивания (по умолчанию)
    FCStateT& operator=(const FCStateT &other) = default;
    /// @brief Оператор перемещающего присваивания (по умолчанию)
    FCStateT& operator=(FCStateT &&other) noexcept = default;
    /// @brief Копирующий конструктор (по умолчанию)
    FCStateT(const FCStateT &other) = default;
    /// @brief Перемещающий конструктор (по умолчанию)
    FCStateT(FCStateT &&other) noexcept = default;

    /// @brief Присвоить состояние из другого FCStateT<> с ДРУГИМИ шаблонными параметрами
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    /// @return Ссылка на текущий объект для цепочки вызовов
    /// @details Копирует ТОЛЬКО общие типы, уникальные игнорируются
    template<typename... OtherStates>
    inline FCStateT& operator=(const FCStateT<OtherStates...> &other) noexcept
    {
        mergeFrom(other);
        return *this;
    }

    // ========================================================================
    // МЕТОДЫ ПРОВЕРКИ СОСТОЯНИЙ
    // ========================================================================

    /// @brief Проверить одно состояние
    /// @tparam State Тип состояния для проверки
    /// @param value Ожидаемое значение
    /// @return true если текущее значение совпадает с value
    /// @details Вызывает static_assert при передаче недопустимого типа.
    template<typename State>
    inline bool is(State value) const noexcept
    {
        static_assert((std::is_same_v<State, States> || ...),
                      "State type is not included in this FCStateT instance");
        return std::get<State>(_values) == value;
    }

    /// @brief Проверить несколько состояний одновременно (логическое И)
    /// @tparam First Первый тип состояния
    /// @tparam Rest Остальные типы состояний
    /// @param first Значение первого состояния
    /// @param rest Значения остальных состояний
    /// @return true если ВСЕ указанные состояния совпадают
    /// @details Использует short-circuit evaluation для оптимизации.
    template<typename First, typename... Rest>
    [[nodiscard]] inline bool is(First first, Rest... rest) const noexcept
    {
        if (!is(first)) return false;
        if constexpr (sizeof...(Rest) > 0) {
            return is(rest...);
        }
        return true;
    }

    // ========================================================================
    // МЕТОДЫ УСТАНОВКИ СОСТОЯНИЙ
    // ========================================================================

    /// @brief Установить одно состояние
    /// @tparam State Тип состояния (должен быть в шаблоне FCStateT<States...>)
    /// @param value Новое значение состояния
    /// @return Ссылка на текущий объект для fluent-интерфейса
    /// @note Вызывает static_assert если тип не входит в список шаблонных параметров
    template<typename State>
    inline FCStateT& set(State value)
    {
        static_assert((std::is_same_v<State, States> || ...),
                      "State type is not included in this FCStateT instance. "
                      "Check your FCStateT<...> template parameters.");
        std::get<State>(_values) = value;
        return *this;
    }

    /// @brief Установить несколько состояний сразу (вариативный шаблон)
    /// @tparam First Первый тип состояния
    /// @tparam Rest Остальные типы состояний
    /// @param first Первое значение
    /// @param rest Остальные значения
    /// @return Ссылка на текущий объект для fluent-интерфейса
    /// @note Рекурсивно вызывает set() для каждого аргумента
    template<typename First, typename... Rest>
    inline FCStateT& set(First first, Rest... rest)
    {
        set(first);
        if constexpr (sizeof...(Rest) > 0) {
            set(rest...);
        }
        return *this;
    }

    // ========================================================================
    // ГЕТТЕРЫ
    // ========================================================================

    /// @brief Получить значение состояния по типу
    /// @tparam State Тип запрашиваемого состояния
    /// @return Текущее значение состояния
    /// @details Вызывает static_assert при недопустимом типе.
    template<typename State>
    [[nodiscard]] inline State get() const noexcept
    {
        static_assert((std::is_same_v<State, States> || ...),
                      "State type is not included in this FCStateT instance");
        return std::get<State>(_values);
    }

    // ========================================================================
    // МЕТОДЫ СРАВНЕНИЯ СОСТОЯНИЙ
    // ========================================================================

    /// @brief Проверить полное равенство ВСЕХ состояний
    /// @param other Объект для сравнения
    /// @return true если все поля совпадают
    [[nodiscard]] inline bool operator==(const FCStateT &other) const noexcept
    {
        return ((std::get<States>(_values) == std::get<States>(other._values)) && ...);
    }

    /// @brief Проверить неравенство (хотя бы одно состояние отличается)
    /// @param other Объект для сравнения
    /// @return true если хотя бы одно поле различается
    [[nodiscard]] inline bool operator!=(const FCStateT &other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Проверить совпадение только указанных типов состояний
    /// @tparam StateTypes Явно указанный список типов для проверки
    /// @param other Объект для сравнения
    /// @return true если все указанные типы совпадают
    template<typename... StateTypes>
    [[nodiscard]] inline bool matches(const FCStateT &other) const noexcept
    {
        static_assert((std::is_same_v<StateTypes, States> && ...),
                      "All StateTypes must be included in this FCStateT instance");
        return ((std::get<StateTypes>(_values) == std::get<StateTypes>(other._values)) && ...);
    }

    /// @brief Проверить, отличается ли хотя бы одно из указанных состояний
    /// @tparam StateTypes Явно указанный список типов для проверки
    /// @param other Объект для сравнения
    /// @return true если хотя бы один указанный тип различается
    template<typename... StateTypes>
    [[nodiscard]] inline bool differs(const FCStateT &other) const noexcept
    {
        static_assert((std::is_same_v<StateTypes, States> && ...),
                      "All StateTypes must be included in this FCStateT instance");
        return ((std::get<StateTypes>(_values) != std::get<StateTypes>(other._values)) || ...);
    }

    /// @brief Проверить, "включает" ли текущее состояние другое (по указанным типам)
    /// @tparam StateTypes Явно указанный список типов для проверки
    /// @param other Объект для сравнения
    /// @return true если все указанные типы совпадают
    template<typename... StateTypes>
    [[nodiscard]] inline bool includes(const FCStateT &other) const noexcept
    {
        static_assert((std::is_same_v<StateTypes, States> && ...),
                      "All StateTypes must be included in this FCStateT instance");
        return ((std::get<StateTypes>(_values) == std::get<StateTypes>(other._values)) && ...);
    }

    /// @brief Получить строковое представление различий между состояниями
    /// @param other Объект для сравнения
    /// @return Строка с описанием различий или пустая строка при равенстве
    [[nodiscard]] inline QString diffDetails(const FCStateT &other) const noexcept
    {
        if (*this == other) return QString();
        return QStringLiteral("states differ");
    }

    // ========================================================================
    // МЕТОДЫ ОБЪЕДИНЕНИЯ СОСТОЯНИЙ (для разных инстанций FCStateT<>)
    // ========================================================================

    /// @brief Объединить состояния из другого FCStateT<> с ДРУГИМИ шаблонными параметрами
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    /// @details Копирует ТОЛЬКО общие типы, уникальные игнорируются
    template<typename... OtherStates>
    inline void mergeFrom(const FCStateT<OtherStates...> &other) noexcept
    {
        (copyIfCommon<States>(other), ...);
    }

    /// @brief Объединить с присваиванием (синтаксический сахар для mergeFrom)
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    /// @return Ссылка на текущий объект для цепочки вызовов
    template<typename... OtherStates>
    inline FCStateT& mergeAssign(const FCStateT<OtherStates...> &other) noexcept
    {
        mergeFrom(other);
        return *this;
    }

    /// @brief Оператор &= для объединения с другим FCStateT<> (любым набором типов)
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    /// @return Ссылка на текущий объект
    /// @details Копирует ТОЛЬКО общие типы — безопасно для разных шаблонов!
    template<typename... OtherStates>
    inline FCStateT& operator&=(const FCStateT<OtherStates...> &other) noexcept
    {
        return mergeAssign(other);
    }

    /// @brief Оператор & для создания нового объединённого состояния
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    /// @return Новый объект с объединёнными общими типами
    template<typename... OtherStates>
    [[nodiscard]] inline FCStateT operator&(const FCStateT<OtherStates...> &other) const noexcept
    {
        FCStateT result = *this;
        result.mergeFrom(other);
        return result;
    }

    // ========================================================================
    // УСЛОВНОЕ ОБЪЕДИНЕНИЕ СОСТОЯНИЙ
    // ========================================================================

    /// @brief Объединить состояния только если они НЕ равны значению по умолчанию
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @tparam State Тип для сравнения с дефолтом
    /// @param other Источник данных
    /// @param defaultValue Значение, которое считается "по умолчанию"
    template<typename... OtherStates, typename State>
    inline void mergeIfNotDefault(const FCStateT<OtherStates...> &other, State defaultValue) noexcept
    {
        (copyIfNotDefault<States>(other, defaultValue), ...);
    }

    /// @brief Объединить состояния только для указанных типов (фильтр)
    /// @tparam FilterTypes Список разрешённых к копированию типов
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    template<typename... FilterTypes, typename... OtherStates>
    inline void mergeOnly(const FCStateT<OtherStates...> &other) noexcept
    {
        (copyIfInFilter<FilterTypes, States>(other), ...);
    }

    /// @brief Объединить состояния с приоритетом: брать значение из other ТОЛЬКО если оно "важнее"
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @tparam PriorityFunc Функтор сравнения приоритетов (bool current, bool other)
    /// @param other Источник данных
    /// @param priorityFunc Предикат приоритета
    template<typename... OtherStates, typename PriorityFunc>
    inline void mergeWithPriority(const FCStateT<OtherStates...> &other, PriorityFunc&& priorityFunc) noexcept
    {
        (copyWithPriority<States>(other, priorityFunc), ...);
    }

    // ========================================================================
    // УДОБНЫЕ МЕТОДЫ ДЛЯ ЧАСТЫХ СЦЕНАРИЕВ СЛИЯНИЯ
    // ========================================================================

    /// @brief Скопировать только состояние ошибки (если есть)
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    /// @note Не перезаписывает "нет ошибки" на существующую ошибку
    template<typename... OtherStates>
    inline void mergeErrorsFrom(const FCStateT<OtherStates...> &other) noexcept
    {
        if constexpr (hasState<FCErrorType>() &&
                      (std::is_same_v<FCErrorType, OtherStates> || ...)) {
            if (hasError() && !other.hasError()) {
                // Не сбрасывать ошибку на "нет ошибки"
            } else if (!hasError() && other.hasError()) {
                set(other.template get<FCErrorType>());
            } else if (other.hasError()) {
                // Если в other есть ошибка — всегда брать её
                set(other.template get<FCErrorType>());
            }
        }
    }

    /// @brief Скопировать только состояние готовности
    /// @tparam OtherStates Набор типов в исходном состоянии
    /// @param other Источник данных
    template<typename... OtherStates>
    inline void mergeReadinessFrom(const FCStateT<OtherStates...> &other) noexcept
    {
        if constexpr (hasState<FCReadyState>() &&
                      (std::is_same_v<FCReadyState, OtherStates> || ...)) {
            if (!isReady() && other.isReady()) {
                set(FCReadyState::Ready);
            } else if (isReady() && !other.isReady()) {
                set(FCReadyState::NotReady);
            }
        }
    }

    /// @brief Сбросить состояние к значениям по умолчанию для указанных типов
    /// @tparam ResetTypes Список типов для сброса
    template<typename... ResetTypes>
    inline void resetStates() noexcept
    {
        (resetSingleState<ResetTypes>(), ...);
    }

    // ========================================================================
    // ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    // ========================================================================

    /// @brief Проверяет, содержит ли экземпляр указанный тип состояния
    /// @tparam State Тип для проверки
    /// @return true если State входит в States...
    /// @note constexpr для использования в static_assert
    template<typename State>
    static constexpr bool hasState() noexcept
    {
        return sizeof...(States) > 0 && (std::is_same_v<State, States> || ...);
    }

    /// @brief Возвращает количество типов состояний в шаблоне
    /// @return Размер пакета States...
    static constexpr std::size_t stateCount() noexcept
    {
        return sizeof...(States);
    }

    // ========================================================================
    // CONVENIENCE-МЕТОДЫ (для частых проверок)
    // ========================================================================

    /// @brief Проверить готовность (FCReadyState::Ready)
    /// @return true если FCReadyState::Ready, иначе false
    [[nodiscard]] inline bool isReady() const noexcept
    {
        if constexpr (hasState<FCReadyState>()) {
            return is(FCReadyState::Ready);
        }
        return false;
    }

    /// @brief Проверить ошибку (FCErrorType != None)
    /// @return true если FCErrorType != None
    [[nodiscard]] inline bool hasError() const noexcept
    {
        if constexpr (hasState<FCErrorType>()) {
            return isNot(FCErrorType::None);
        }
        return false;
    }

    /// @brief Проверить активное выполнение (FCPlayState::Start)
    /// @return true если FCPlayState::Start
    [[nodiscard]] inline bool isPlaying() const noexcept
    {
        if constexpr (hasState<FCPlayState>()) {
            return is(FCPlayState::Start);
        }
        return false;
    }

    /// @brief Проверить паузу (FCPlayState::Pause)
    /// @return true если FCPlayState::Pause
    [[nodiscard]] inline bool isPaused() const noexcept
    {
        if constexpr (hasState<FCPlayState>()) {
            return is(FCPlayState::Pause);
        }
        return false;
    }

    /// @brief Проверить, что состояние НЕ равно указанному значению
    /// @tparam State Тип состояния
    /// @param value Значение для проверки на неравенство
    /// @return true если текущее значение != value
    template<typename State>
    [[nodiscard]] inline bool isNot(State value) const noexcept
    {
        static_assert((std::is_same_v<State, States> || ...),
                      "State type is not included in this FCStateT instance");
        return std::get<State>(_values) != value;
    }

    /// @brief Проверить состояние как ЛЮБОЕ из перечисленных (логическое ИЛИ)
    /// @tparam State Тип состояния
    /// @param value1 Первое допустимое значение
    /// @param value2 Второе допустимое значение
    /// @return true если текущее значение равно value1 или value2
    template<typename State>
    [[nodiscard]] inline bool isAny(State value1, State value2) const noexcept
    {
        static_assert((std::is_same_v<State, States> || ...),
                      "State type is not included in this FCStateT instance");
        return (std::get<State>(_values) == value1) || (std::get<State>(_values) == value2);
    }

    /// @brief Проверить состояние как ЛЮБОЕ из трёх значений (логическое ИЛИ)
    /// @tparam State Тип состояния
    /// @param value1 Первое допустимое значение
    /// @param value2 Второе допустимое значение
    /// @param value3 Третье допустимое значение
    /// @return true если текущее значение совпадает с любым из трёх
    template<typename State>
    [[nodiscard]] inline bool isAny(State value1, State value2, State value3) const noexcept
    {
        static_assert((std::is_same_v<State, States> || ...),
                      "State type is not included in this FCStateT instance");
        return (std::get<State>(_values) == value1) ||
               (std::get<State>(_values) == value2) ||
               (std::get<State>(_values) == value3);
    }

    /// @brief Отладочный метод: возвращает строку с именами типов в шаблоне
    /// @return Строка вида "FCStateT<FCReadyState, FCErrorType>"
    [[nodiscard]] inline QString debugTypeNames() const noexcept
    {
        QString result = QStringLiteral("FCStateT<");
        bool first = true;

        auto addIfHas = [&result, &first](const char* name, bool has) {
            if(has) {
                if(!first) result += QStringLiteral(", ");
                result += QString::fromUtf8(name);
                first = false;
            }
        };

        addIfHas("FCReadyState", hasState<FCReadyState>());
        addIfHas("FCPlayState", hasState<FCPlayState>());
        addIfHas("FCErrorType", hasState<FCErrorType>());
        addIfHas("FCPanelState", hasState<FCPanelState>());
        addIfHas("FCVisibilityState", hasState<FCVisibilityState>());

        result += QStringLiteral(">");
        return result;
    }

private:
    // Вспомогательная функция: копирует значение, если тип есть в обоих шаблонах
    template<typename State, typename... OtherStates>
    inline void copyIfCommon(const FCStateT<OtherStates...> &other) noexcept
    {
        if constexpr ((std::is_same_v<State, OtherStates> || ...)) {
            std::get<State>(_values) = std::get<State>(other._values);
        }
        return;
    }

    // Вспомогательная функция: копирует только если значение != defaultValue
    template<typename State, typename... OtherStates>
    inline void copyIfNotDefault(const FCStateT<OtherStates...> &other, State defaultValue) noexcept
    {
        if constexpr ((std::is_same_v<State, OtherStates> || ...)) {
            const auto &otherValue = std::get<State>(other._values);
            if (otherValue != defaultValue) {
                std::get<State>(_values) = otherValue;
            }
        }
        return;
    }

    // Вспомогательная функция: копирует только если тип есть в списке фильтра
    template<typename... FilterTypes, typename State, typename... OtherStates>
    inline void copyIfInFilter(const FCStateT<OtherStates...> &other) noexcept
    {
        if constexpr ((std::is_same_v<State, OtherStates> || ...) &&
                      (std::is_same_v<State, FilterTypes> || ...)) {
            std::get<State>(_values) = std::get<State>(other._values);
        }
        return;
    }

    // Вспомогательная функция: копирует с приоритетом (через предикат)
    template<typename State, typename... OtherStates, typename PriorityFunc>
    inline void copyWithPriority(const FCStateT<OtherStates...> &other, PriorityFunc&& priorityFunc) noexcept
    {
        if constexpr ((std::is_same_v<State, OtherStates> || ...)) {
            const auto &currentValue = std::get<State>(_values);
            const auto &otherValue = std::get<State>(other._values);
            if (priorityFunc(currentValue, otherValue)) {
                std::get<State>(_values) = otherValue;
            }
        }
        return;
    }

    // Вспомогательная функция для сброса состояния
    template<typename State>
    inline void resetSingleState() noexcept
    {
        if constexpr (hasState<State>()) {
            // Первый элемент enum — значение по умолчанию
            std::get<State>(_values) = static_cast<State>(0);
        }
        return;
    }

    // Вспомогательная функция для operator& (для одинаковых шаблонов)
    template<size_t... I>
    inline void combineStates(FCStateT &result, const FCStateT &other,
                              std::index_sequence<I...>) noexcept
    {
        ((std::get<I>(result._values) = std::get<I>(other._values)), ...);
    }

    /// @brief Внутреннее хранилище значений состояний
    /// @details Tuple упорядочен по типам шаблонных параметров States...
    std::tuple<States...> _values;
};

// ============================================================================
// TYPE ALIASES ДЛЯ УДОБСТВА (с защитой от ошибок через static_assert)
// ============================================================================
/// @brief Состояние дисплея (UI-слой — может включать FCErrorState)
using FCDisplayState = FCStateT<FCReadyState, FCPlayState, FCErrorType, FCPanelState, FCVisibilityState>;

/// @brief Состояние устройства (стандартный набор для логики — БЕЗ FCErrorState!)
using FCDeviceState = FCStateT<FCReadyState, FCPlayState, FCErrorType>;

/// @brief Состояние контейнера SVG (только логические типы)
using FCSVGImageContainerState = FCStateT<FCReadyState, FCPlayState, FCErrorType>;

///// @brief Состояние аппаратного слоя плоттера (только логические типы)
//using FCPlotterHardwareState = FCStateT<FCReadyState, FCPlayState, FCErrorType>;

/// @brief Состояние плоттера (алиас на стандартное состояние устройства)
using FCPlotterState = FCDeviceState;

/// @brief Состояние контроллера Marlin (только логические типы)
using FCMarlinControllerState = FCDeviceState;

/// @brief Состояние очереди команд плоттера
using FCQueueCondition = FCStateT<FCReadyState, FCPlayState>;

// ============================================================================
// TYPE TRAITS: Проверка совместимости для слияния состояний
// ============================================================================

/// @brief Проверить, содержится ли тип T в списке типов Types...
/// @note constexpr для использования в compile-time проверках
template<typename T, typename... Types>
constexpr bool contains_type = (std::is_same_v<T, Types> || ...);

/// @brief Проверить, есть ли хотя бы один общий тип между двумя наборами состояний
/// @tparam StatesA Первый набор типов
/// @tparam StatesB Второй набор типов
/// @return true если хотя бы один тип из StatesA есть в StatesB
template<typename... StatesA, typename... StatesB>
constexpr bool hasCommonState() noexcept
{
    if constexpr (sizeof...(StatesA) == 0 || sizeof...(StatesB) == 0) {
        return false;
    } else {
        return (contains_type<StatesA, StatesB...> || ...);
    }
}

/// @brief Макрос для удобной проверки совместимости в compile-time
/// @example: FC_ASSERT_MERGE_COMPATIBLE(FCDeviceState, FCPlotterHardwareState);
#define FC_ASSERT_MERGE_COMPATIBLE(StateA, StateB) \
    static_assert( \
        hasCommonState<typename StateA::States..., typename StateB::States...>(), \
        #StateA " and " #StateB " have no common state types — merge will do nothing!" \
    )

// ============================================================================
// ENUM для направлений свайпа (в namespace FC)
// ============================================================================

namespace FC
{
    /// @brief Направление жеста свайпа для навигации по интерфейсу
    enum class SwipeDirection
    {
        None,   ///< Жест не распознан
        Left,   ///< Свайп влево
        Right,  ///< Свайп вправо
        Up,     ///< Свайп вверх
        Down    ///< Свайп вниз
    };
}

#endif // FC_STATE_H
