#ifndef FC_DISPLAY_H
#define FC_DISPLAY_H

#include <QMainWindow>
#include <QSettings>
#include <QGridLayout>
#include <QPushButton>
#include <QFileInfo>
#include <QTimer>
#include <QPair>
#include <QRect>
#include <QPoint>
#include <QFileSystemWatcher>

#include "FCConfigFile.h"
#include "FCState.h"
#include "FCYesNoDialog.h"
#include "FCErrorDialog.h"
#include "FC2DSize.h"

namespace Ui { class FCDisplay; }

/**
 * @class FCDisplay
 * @brief Главное окно графического интерфейса системы управления плоттером (встраиваемая система).
 *
 * Класс реализует сенсорный интерфейс управления плоттером с поддержкой:
 * - Переключения между панелями (Main / Statistics / ScreenSaver)
 * - Горизонтальных свайпов для навигации
 * - Заставки с автоматическим включением при бездействии
 * - Динамического построения сетки моделей и меню плоттеров
 * - Консоли логов с ограниченным буфером
 * - Состояний воспроизведения (Play/Pause/Stop) через шаблонный класс FCStateT<>
 *
 * Архитектурные особенности:
 * - Все состояния хранятся в FCState (type alias для FCStateT<все состояния>)
 * - Сигналы эмитятся при изменении состояний (для синхронизации с FCMachine)
 * - Свайпы обрабатываются через переопределённый event() и eventFilter()
 * - _previousPanel хранит последнюю РАБОЧУЮ панель для возврата из скринсейвера
 * - Дочерние виджеты (QTreeView, QScrollArea) перехватывают события через фильтр
 *
 * @threadsafe Нет (используется в основном потоке GUI)
 * @see FCStateT, FCMachine, FCSVGImageParser
 */

class FCDisplay
    : public QMainWindow
{
Q_OBJECT
public:
    /// @brief Состояние дисплея (UI-слой — может включать FCErrorState)
    using FCDisplayState = FCStateT<FCReadyState, FCPlayState, FCErrorType, FCPanelState, FCVisibilityState>;

    /**
     * @brief Конструктор класса.
     * @param parent Родительский виджет (по умолчанию nullptr).
     *
     * Инициализирует:
     * - Интерфейс через Ui::FCDisplay
     * - Объект состояния _state (FCStateT<>)
     * - Таймер заставки _screenSaverTimer
     * - Переменные для обработки свайпов (_touchStartPos, _isTouching)
     * - Вызывает init() для полной настройки интерфейса
     */
    explicit FCDisplay(QWidget *parent = nullptr);

    /**
     * @brief Деструктор класса.
     *
     * Освобождает ресурсы:
     * - Удаляет объект интерфейса _ui
     * - Остальные члены (с указателями) удаляются автоматически или принадлежат родительским объектам
     * - _state уничтожается автоматически (на стеке, не требует delete)
     */
    ~FCDisplay() override;

    /**
     * @brief Оператор присваивания удалён.
     *
     * Класс не поддерживает копирование или присваивание, так как владеет ресурсами
     * (указатели на виджеты, таймеры, диалоги), которые нельзя безопасно дублировать.
     */
    FCDisplay& operator=(const FCDisplay&) = delete;

    // ПРОКСИ-МЕТОДЫ ДЛЯ ДОСТУПА К СОСТОЯНИЮ (через FCStateT<>)

    /**
     * @brief Проверка состояния готовности системы.
     * @param state Состояние для проверки (FCReadyState).
     * @return true, если состояние активно.
     *
     * @note Метод [[nodiscard]] требует обработки возвращаемого значения.
     * @note noexcept — метод не генерирует исключений.
     * @note Использует шаблонный метод is<FCReadyState>() из FCStateT<>
     */
    [[nodiscard]] inline bool is(FCReadyState state) const noexcept { return _state.is<FCReadyState>(state); }

    /**
     * @brief Проверка состояния воспроизведения.
     * @param state Состояние для проверки (FCPlayState: Stop/Start/Pause).
     * @return true, если состояние активно.
     *
     * @note Используется для блокировки/разблокировки элементов управления.
     */
    [[nodiscard]] inline bool is(FCPlayState state) const noexcept { return _state.is<FCPlayState>(state); }

//    /**
//     * @brief Проверка состояния изменений.
//     * @param state Состояние для проверки (FCChangedState).
//     * @return true, если состояние активно.
//     *
//     * @note Используется для подтверждения выхода при несохранённых изменениях.
//     */
//    [[nodiscard]] inline bool is(FCChangedState state) const noexcept { return _state.is<FCChangedState>(state); }

    /**
     * @brief Проверка типа ошибки.
     * @param type Тип ошибки для проверки (FCErrorType).
     * @return true, если ошибка активна.
     *
     * @note Critical ошибки блокируют интерфейс до подтверждения пользователем.
     */
    [[nodiscard]] inline bool is(FCErrorType type) const noexcept { return _state.is<FCErrorType>(type); }

    /**
     * @brief Проверка активной панели интерфейса.
     * @param panel Панель для проверки (FCPanelState: Main/Statistics/ScreenSaver).
     * @return true, если панель активна.
     *
     * @note Используется для условного включения/выключения элементов интерфейса.
     */
    [[nodiscard]] inline bool is(FCPanelState panel) const noexcept { return _state.is<FCPanelState>(panel); }

    /**
     * @brief Проверка видимости элементов интерфейса.
     * @param visibility Состояние видимости (FCVisibilityState: Show/Hide).
     * @return true, если видимость активна.
     *
     * @note Используется для управления геометрией консоли логов.
     */
    [[nodiscard]] inline bool is(FCVisibilityState visibility) const noexcept { return _state.is<FCVisibilityState>(visibility); }

    /**
     * @brief Установка состояния готовности.
     * @param state Новое состояние (FCReadyState).
     *
     * @note Эмитит сигнал readyStateChanged() при изменении.
     * @note Использует шаблонный метод set<FCReadyState>() из FCStateT<>
     */
    inline void set(FCReadyState state);

    /**
     * @brief Установка состояния воспроизведения.
     * @param state Новое состояние (FCPlayState).
     *
     * @note Автоматически обновляет иконки кнопок Play/Pause/Stop.
     * @note Эмитит сигнал playStateChanged() при изменении.
     */
    inline void set(FCPlayState state);

    /**
     * @brief Установка состояния изменений.
     * @param state Новое состояние (FCChangedState).
     *
     * @note Эмитит сигнал changedStateChanged() при изменении.
     */
    inline void set(FCChangedState state);

    /**
     * @brief Установка типа ошибки.
     * @param type Тип ошибки (FCErrorType).
     *
     * @note Ошибки отображаются в консоли и могут блокировать интерфейс.
     * @note Эмитит сигнал errorTypeChanged() при изменении.
     */
    inline void set(FCErrorType type);

    /**
     * @brief Установка активной панели интерфейса.
     * @param panel Новая панель (FCPanelState).
     *
     * @note Переключает QStackedWidget и обновляет доступность элементов управления.
     * @note Эмитит сигнал panelStateChanged() при изменении.
     */
    inline void set(FCPanelState panel);

    /**
     * @brief Установка видимости элементов.
     * @param visibility Новое состояние видимости (FCVisibilityState).
     *
     * @note Используется для показа/скрытия консоли логов.
     * @note Эмитит сигнал visibilityStateChanged() при изменении.
     */
    inline void set(FCVisibilityState visibility);

//    /**
//     * @brief Получение полного объекта состояния.
//     * @return Константная ссылка на текущий FCState.
//     *
//     * @note Используется для чтения всех состояний сразу (например, при сериализации).
//     * @note FCState — это type alias для FCStateT<все 6 состояний>
//     */
//    [[nodiscard]] inline const FCStateT& state() const noexcept { return _state; }

public slots:
    /**
     * @brief Обработка входящего сообщения (логирование).
     * @param name Имя источника сообщения (например, "System", "Parser").
     * @param message Текст сообщения.
     *
     * @note Добавляет сообщение в консоль с временной меткой.
     * @note Если консоль скрыта — автоматически показывает её.
     * @note Сбрасывает таймер заставки (признак активности пользователя).
     */
    void onMessage(const QString &name, const QString &message);

    /**
     * @brief Обработка ошибки (обёртка над onMessage).
     * @param name Имя источника ошибки.
     * @param message Текст ошибки.
     *
     * @note Автоматически добавляет префикс "Ошибка: " к сообщению.
     */
    inline void onError(const QString &name, const QString &message) { onMessage(name, "Ошибка: " + message); }

    /**
     * @brief Обновление индикатора прогресса парсинга модели.
     * @param percent Процент выполнения (0–100).
     *
     * @note Показывает/скрывает progressBar при необходимости.
     * @note Автоматически скрывает прогресс-бар после завершения (100%).
     */
    void onParsingProcess(int percent);

private slots:
    /**
     * @brief Переключение видимости консоли логов.
     *
     * @note Меняет геометрию консоли и прогресс-бара.
     * @note Сбрасывает таймер заставки.
     */
    void onConsole();

    /**
     * @brief Обработка выбора модели из сетки.
     *
     * @note Обновляет иконку и имя выбранной модели.
     * @note Загружает метаданные файла (размер, дата изменения).
     * @note Разблокирует кнопку Play при успешном выборе.
     */
    void onSelectModelsElement();

    /**
     * @brief Обработка нажатия кнопки Play/Pause.
     *
     * Логика:
     * - Если остановлено/пауза → запускает воспроизведение (сигнал play())
     * - Если воспроизводится → ставит на паузу (сигнал pause())
     * - Блокирует выбор модели во время воспроизведения
     */
    void onPlayPausePressed();

    /**
     * @brief Переключение между подключёнными плоттерами.
     *
     * @note Циклически переключает вкладки в QTabWidget.
     * @note Сбрасывает таймер заставки.
     */
    void onChangePlotterPressed();

    /**
     * @brief Отображение метаданных выбранной модели.
     * @param fileInfo Информация о файле модели.
     *
     * @note Показывает: имя файла, дату изменения, размер (в байтах/КБ/МБ).
     */
    void onModelDataShow(const QFileInfo &fileInfo);

    /**
     * @brief Переключение на указанную панель интерфейса.
     * @param panel Целевая панель (FCPanelState).
     *
     * Ключевая логика:
     * - Если panel != ScreenSaver → сохраняет в _previousPanel и запускает таймер
     * - Переключает QStackedWidget только если текущий индекс отличается
     * - Обновляет состояние через set(panel) для синхронизации
     *
     * @note _previousPanel НЕ обновляется при переходе в ScreenSaver!
     */
    void onSwitchToPanel(const FCPanelState panel);

    /**
     * @brief Обработка вывода сообщения в статусную строкувыбора модели из сетки.
     *
     * @note Выводит имя отправителя и сообщение в статусную строку.
     */
    void onStatusLineOut(const QString &name, const QString &statusMessage);

    /**
     * @brief Обработка изменения содержимого папки с моделями.
     *
     * @note ищет все добавленные или удаленные файлы моделей и синхронизирует список моделей.
     */
    void onDirectoryChanged(const QString &path);

    /**
     * @brief Добавление нового плоттера в систему.
     * @param name Имя/серийный номер плоттера.
     *
     * @note Проверяет дубликаты через FCConfigFile.
     * @note Перестраивает меню плоттеров при добавлении нового.
     * @note Логирует событие в консоль.
     */
    void addPlotterName(const QString &name);

    /**
     * @brief Обновление интерфейса при смене панели.
     * @param panel Новая активная панель.
     *
     * @note Включает/выключает элементы управления в зависимости от панели.
     * @note Управляет таймером заставки (старт/стоп).
     */
    void update(FCPanelState panel);

    /**
     * @brief Обновление интерфейса при смене состояния воспроизведения.
     * @param state Новое состояние (Stop/Start/Pause).
     *
     * @note Меняет иконку кнопки Play/Pause.
     * @note Блокирует/разблокирует выбор модели и кнопки управления.
     */
    void update(FCPlayState state);

signals:
    /**
     * @brief Сигнал об изменении состояния готовности.
     * @param state Новое состояние FCReadyState.
     *
     * @note Эмитится при вызове set(FCReadyState) если значение изменилось.
     * @note Используется для синхронизации с FCMachine.
     */
    void readyStateChanged(FCReadyState state);

    /**
     * @brief Сигнал об изменении состояния воспроизведения.
     * @param state Новое состояние FCPlayState.
     *
     * @note Эмитится при вызове set(FCPlayState) если значение изменилось.
     */
    void playStateChanged(FCPlayState state);

    /**
     * @brief Сигнал об изменении состояния изменений.
     * @param state Новое состояние FCChangedState.
     *
     * @note Эмитится при вызове set(FCChangedState) если значение изменилось.
     */
    void changedStateChanged(FCChangedState state);

    /**
     * @brief Сигнал об изменении типа ошибки.
     * @param type Новый тип FCErrorType.
     *
     * @note Эмитится при вызове set(FCErrorType) если значение изменилось.
     */
    void errorTypeChanged(FCErrorType type);

    /**
     * @brief Сигнал об изменении активной панели.
     * @param panel Новая панель FCPanelState.
     *
     * @note Эмитится при вызове set(FCPanelState) если значение изменилось.
     */
    void panelStateChanged(FCPanelState panel);

    /**
     * @brief Сигнал об изменении видимости элементов.
     * @param visibility Новое состояние FCVisibilityState.
     *
     * @note Эмитится при вызове set(FCVisibilityState) если значение изменилось.
     */
    void visibilityStateChanged(FCVisibilityState visibility);

    /**
     * @brief Сигнал запуска воспроизведения модели.
     * @param model Имя модели для воспроизведения.
     *
     * @note Обрабатывается внешним контроллером (FCMachine → FCSVGImageParser).
     */
    void play(const QString &model);

    /**
     * @brief Сигнал паузы воспроизведения.
     *
     * @note Обрабатывается внешним контроллером.
     */
    void pause();

    /**
     * @brief Сигнал остановки воспроизведения.
     *
     * @note Обрабатывается внешним контроллером.
     * @note Требует подтверждения через FCYesNoDialog.
     */
    void stop();

protected:
    /**
     * @brief Переопределённый обработчик событий Qt.
     * @param event Указатель на событие.
     * @return true, если событие обработано; иначе передаётся базовому классу.
     *
     * Обрабатывает:
     * - Начало/конец касания для детектирования свайпов
     * - Горизонтальные свайпы для переключения панелей
     * - Выход из скринсейвера по касанию/клику/нажатию клавиши
     * - Сброс таймера заставки при любой активности
     *
     * @note Возвращает true для "потреблённых" событий, чтобы остановить дальнейшую обработку.
     * @see eventFilter()
     */
    bool event(QEvent *event) override;

    /**
     * @brief Фильтр событий для перехвата касаний от дочерних виджетов.
     * @param obj Объект-источник события.
     * @param event Указатель на событие.
     * @return Результат обработки (true = событие потреблено).
     *
     * @note Решает проблему перехвата событий QTreeView/QScrollArea.
     * @note Перенаправляет события касания в основной метод event().
     * @note Пропускает события кнопок и элементов управления для их нормальной работы.
     * @see event()
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // КОНСТАНТЫ ГЕОМЕТРИИ ИНТЕРФЕЙСА
    /// @brief Размер кнопки модели в сетке (ширина × высота, пиксели).
    static constexpr const FC2DSize DefaultModelButtonSize{120, 145};

    /// @brief Размер иконки внутри кнопки модели.
    static constexpr const FC2DSize DefaultModelIconSize{95, 120};

    /// @brief Отступы между элементами в сетке моделей (горизонтальный, вертикальный).
    static constexpr const FC2DSize DefaultModelSpacing{8, 15};

    /**
     * @brief Геометрия консоли логов: {видимая, скрытая}.
     * @note QRect(x, y, width, height)
     * @note Первая пара: консоль показана (занимает нижнюю часть экрана)
     * @note Вторая пара: консоль скрыта (сдвинута за пределы видимости)
     */
    static constexpr const QPair<QRect, QRect> DefaultConsoleSize
    {
        {390, 60, 841, 951},   // Показана
        {390, 540, 841, 471}   // Скрыта (сдвинута вниз)
    };

    /**
     * @brief Геометрия прогресс-бара: {обычное, при показанной консоли}.
     * @note Первая пара: прогресс-бар в обычном положении
     * @note Вторая пара: прогресс-бар сдвинут вверх при показанной консоли
     */
    static constexpr const QPair<QRect, QRect> DefaultProgressBarSize
    {
        {10, 540, 361, 41},    // Обычное положение
        {10, 790, 361, 41}     // Сдвинуто при показанной консоли
    };

    /// @brief Путь к иконке кнопки "Воспроизвести" (ресурс Qt).
    static constexpr const char* DefaultPlayImage = ":/img/play.svg";

    /// @brief Путь к иконке кнопки "Пауза" (ресурс Qt).
    static constexpr const char* DefaultPauseImage = ":/img/pause.svg";

    /// @brief Путь к иконке-заглушке (модель не найдена или ошибка загрузки).
    static constexpr const char* DefaultFNFImage = ":/img/question.svg";

    /// @brief Разрешение дисплея по умолчанию (для встраиваемой системы).
    static constexpr const FC2DSize DefaultDisplayResolution{1280, 1024};

    /// @brief Таймаут заставки в миллисекундах (60 секунд бездействия).
    static constexpr int DefaultScreenSaverTimeOut = 60000 * 3;

//    /// @brief Таймаут опроса директории моделей и добавления их в случае появления новых (интервал - 120 сек).
//    static constexpr int DefaultReBuilderModelsTimeOut = 20000;

    // МЕТОДЫ ИНИЦИАЛИЗАЦИИ
    /// @brief Построение меню плоттеров (вкладки с серийными номерами).
    void buildPlottersMenu();

    /**
     * @brief Построение сетки кнопок моделей.
     *
     * @note Сканирует директорию из FCConfigFile::modelsPath().
     * @note Создаёт QToolButton для каждого файла с расширением модели.
     * @note Настраивает вертикальный скролл + QScroller для тач-свайпа.
     * @note Вызывается ТОЛЬКО ОДИН РАЗ при инициализации (кэшируется через _modelsBuilt).
     * @see buildModelsMenu(), rebuildModels()
     */
    void buildModelsMenu();

    /**
     * @brief Принудительная перестройка списка моделей.
     *
     * @note Сбрасывает флаг _modelsBuilt и вызывает buildModelsMenu().
     * @note Используется при добавлении/удалении файлов моделей во время работы.
     */
    void rebuildModels();

    /// @brief Настройка диалогов подтверждения и ошибок.
    void buildDialogs();

    /**
     * @brief Основная инициализация интерфейса.
     *
     * Выполняет:
     * - Настройку флагов окна (безрамочное, приём тач-событий)
     * - Скрытие/показ элементов по умолчанию
     * - Подключение сигналов/слотов
     * - Инициализацию _previousPanel = FCPanelState::Main
     * - Запуск таймера заставки
     * - Установку фильтров событий на дочерние виджеты
     *
     * @see init(), installTouchFilters()
     */
    void init();

    /// @brief Отображение заглушки при отсутствии моделей.
    void showNoModelsPlaceHolder();

    /**
     * @brief Установка eventFilter на дочерние виджеты для перехвата свайпов.
     *
     * @note Проходит по всем QWidget через findChildren<>().
     * @note Пропускает кнопки и элементы управления (они должны работать).
     * @note Устанавливает атрибут WA_AcceptTouchEvents для propagate событий.
     * @see installTouchFilters(), eventFilter()
     */
    void installTouchFilters();

    // ЧЛЕНЫ ДАННЫХ
    /// @brief Указатель на сгенерированный UI-класс (из .ui файла).
    Ui::FCDisplay *_ui = nullptr;

    /**
     * @brief Централизованное хранилище состояний системы.
     *
     * @note FCState — это type alias для FCStateT<все 6 состояний>
     * @note Не наследуется от QObject — сигналы эмитятся в этом классе
     * @note Копируемый и перемещаемый (хранится на стеке)
     * @see FCReadyState, FCPlayState, FCErrorState, FCErrorType, FCPanelState, FCVisibilityState
     */
    FCDisplayState _state;

    /// @brief Сетка для динамического размещения кнопок моделей.
    QGridLayout _gridOfModels;

    /// @brief Диалог подтверждения "Да/Нет" (для остановки воспроизведения).
    FCYesNoDialog _yn;

    /// @brief Диалог отображения критических ошибок.
    FCErrorDialog _error;

    /**
     * @brief Таймер автоматического включения заставки.
     *
     * @note Перезапускается при любой активности пользователя.
     * @note При срабатывании вызывает onSwitchToPanel(ScreenSaver).
     * @note Останавливается при активации скринсейвера.
     */
    QTimer *_screenSaverTimer = nullptr;

//    /**
//     * @brief Таймер автоматического обновления списка моделей.
//     *
//     * @note Перезапускается по времени.
//     * @note При срабатывании вызывает rebuildModels().
//     */
//    QTimer *_reBuilderModels = nullptr;

    QFileSystemWatcher *_watcher = nullptr;

    /**
     * @brief Последняя РАБОЧАЯ панель (Main или Statistics).
     *
     * Критически важная переменная для навигации:
     * - Обновляется ТОЛЬКО при переходе на рабочую панель
     * - НЕ обновляется при переходе в ScreenSaver
     * - Используется для возврата из скринсейвера (по касанию или свайпу)
     *
     * @note Инициализируется как FCPanelState::Main в init().
     */
    FCPanelState _previousPanel = FCPanelState::Main;

    /// @brief Начальная точка касания для расчёта вектора свайпа.
    QPoint _touchStartPos;

    /**
     * @brief Флаг активного касания.
     *
     * @note true между TouchBegin/TouchEnd (или MousePress/MouseRelease).
     * @note Используется для игнорирования "дрожания" при свайпах.
     * @note Сбрасывается при TouchCancel или после обработки свайпа.
     */
    bool _isTouching = false;

    /**
     * @brief Флаг: модели уже построены.
     *
     * @note true после первого вызова buildModelsMenu().
     * @note Позволяет избежать повторного сканирования диска и пересоздания виджетов.
     * @note Сбрасывается только при явном вызове rebuildModels().
     */
    bool _modelsBuilt = false;

    /**
     * @brief Флаг: плоттеры уже построены.
     *
     * @note true после первого вызова buildPlottersMenu().
     * @note Позволяет избежать повторного пересоздания вкладок.
     * @note Сбрасывается при добавлении нового плоттера через addPlotterName().
     */
    bool _plottersBuilt = false;
};

#endif // FC_DISPLAY_H
