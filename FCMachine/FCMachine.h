#ifndef FC_MACHINE_H
#define FC_MACHINE_H

#include <QApplication>
#include <QTextStream>
#include <QCommandLineParser>
#include <QDebug>

#include "FCState.h"
#include "FCPlotter.h"
#include "FCConfigFile.h"
#include "FCDisplay.h"
#include "FCSVGImageParser.h"
#include "FCSVGImageContainer.h"
// #include "FCSVGImageCoder.h"
// #include "FCPlotter.h"

/// @brief Набор состояний для машины
using FCMachineState = FCStateT<FCReadyState, FCErrorType>;

/**
* @class FCMachine
* @brief Главный класс приложения, объединяющий все компоненты системы
*
* Предназначен для:
* - Инициализации всех подсистем (дисплей, парсер, кодер, плоттер)
* - Настройки взаимодействия между компонентами через сигналы/слоты
* - Управления жизненным циклом приложения
* - Обработки командной строки
*
* @note Состояния управляются через класс FCState (централизованно)
* @threadsafe
* - Основной поток: GUI (FCDisplay)
* - Рабочие потоки: через QtConcurrent
*
* @see FCState, FCDisplay, FCSVGImageParser
*/
class FCMachine
    : public QApplication
{
Q_OBJECT
public:
    /// @brief Стиль приложения (stylesheet для QToolButton)
    const QString StyleSheetMachine = R"(
        QToolButton
        {
            border: 2px solid #8f8f91;
            border-radius: 10px;
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,
                                        stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);
        }
        QToolButton:pressed
        {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #00e34f, stop: 0.4 #00e029,
                                        stop: 0.5 #00bf29, stop: 1.0 #02bd2a);
        }

        QToolButton#ssButton
        {
            background-color: black;
            color: white;
            border: 0px solid black;  /* Граница: толщина | стиль | цвет */
        }
        QToolButton#ssButton:pressed
        {
            background-color: black;
            color: green;
            border: 0px solid black;  /* Граница: толщина | стиль | цвет */
        }
    )";

    /**
    * @brief Конструктор класса
    * @param argc Количество аргументов командной строки
    * @param argv Массив аргументов командной строки
    */
    explicit FCMachine(int argc, char **argv);

    /**
    * @brief Деструктор класса
    */
    ~FCMachine() override = default;

    /**
    * @brief Главный цикл приложения
    * @return Код завершения приложения
    */
    int exec();

    // ПРОКСИ-МЕТОДЫ ДЛЯ ДОСТУПА К СОСТОЯНИЮ (через FCStateT::...)
    /**
    * @brief Проверка состояния готовности системы.
    * @param state Состояние для проверки (FCReadyState).
    * @return true, если состояние активно.
    */
    [[nodiscard]] inline bool is(FCReadyState state) const noexcept { return _state.is(state); }

//    /**
//    * @brief Проверка состояния воспроизведения.
//    * @param state Состояние для проверки (FCPlayState).
//    * @return true, если состояние активно.
//    */
//    [[nodiscard]] inline bool is(FCPlayState state) const noexcept { return _state.is(state); }

//    /**
//    * @brief Проверка состояния изменений.
//    * @param state Состояние для проверки (FCChangedState).
//    * @return true, если состояние активно.
//    */
//    [[nodiscard]] inline bool is(FCChangedState state) const noexcept { return _state.is(state); }

//    /**
//    * @brief Проверка типа ошибки.
//    * @param type Тип ошибки для проверки (FCErrorType).
//    * @return true, если ошибка активна.
//    */
//    [[nodiscard]] inline bool is(FCErrorType type) const noexcept { return _state.is(type); }

//    /**
//    * @brief Проверка активной панели интерфейса.
//    * @param state Состояние панели для проверки (FCPanelState).
//    * @return true, если панель активна.
//    */
//    [[nodiscard]] inline bool is(FCPanelState state) const noexcept { return _state.is(state); }

//    /**
//    * @brief Проверка видимости элементов интерфейса.
//    * @param state Состояние видимости для проверки (FCVisibilityState).
//    * @return true, если видимость активна.
//    */
//    [[nodiscard]] inline bool is(FCVisibilityState state) const noexcept { return _state.is(state); }

    // УСТАНОВКА СОСТОЯНИЙ
    /**
    * @brief Установка состояния готовности.
    * @param state Новое состояние (FCReadyState).
    */
    void set(FCReadyState state)
    {
        _state.set(state);
        emit readyStateChanged(state);
    }

//    /**
//    * @brief Установка состояния воспроизведения.
//    * @param state Новое состояние (FCPlayState).
//    */
//    void set(FCPlayState state)
//    {
//        _state.set(state);
//        emit playStateChanged(state);
//    }

//    /**
//    * @brief Установка состояния изменений.
//    * @param state Новое состояние (FCChangedState).
//    */
//    void set(FCChangedState state)
//    {
//        _state.set(state);
//        emit changedStateChanged(state);
//    }

    /**
    * @brief Установка типа ошибки.
    * @param type Тип ошибки (FCErrorType).
    */
    void set(FCErrorType type)
    {
        _state.set(type);
        emit errorTypeChanged(type);
    }

//    /**
//    * @brief Установка состояния панели.
//    * @param state Новое состояние панели (FCPanelState).
//    */
//    void set(FCPanelState state)
//    {
//        _state.set(state);
//        emit panelStateChanged(state);
//    }

//    /**
//    * @brief Установка состояния видимости.
//    * @param state Новое состояние видимости (FCVisibilityState).
//    */
//    void set(FCVisibilityState state)
//    {
//        _state.set(state);
//        emit visibilityStateChanged(state);
//    }

    /**
    * @brief Получение полного объекта состояния.
    * @return Константная ссылка на текущий FCState.
    */
    [[nodiscard]] inline const FCMachineState state() const noexcept { return _state; }

    /**
    * @brief Установка полного объекта состояния.
    * @param newState Новый объект состояния.
    */
    inline void set(FCMachineState &state) { _state = state; }

private:
    /**
    * @brief Анализ командной строки (Command Line Analyzer)
    * @return true если обработка успешна и приложение должно запуститься
    */
    bool cla();

    /**
    * @brief Инициализация компонентов и связей
    */
    bool init();

    /**
    * @brief Вывод сведений о программе в консоль
    * @return Форматированная строка с информацией
    */
    const QString about();

private slots:
    /**
    * @brief Обработка завершения парсинга
    * @param result Результат парсинга
    */
    void onParsingFinished(const FCSVGImageParser::ParseResult &result);

    /**
    * @brief Обработка сообщений от компонентов
    * @param name Имя компонента
    * @param message Текст сообщения
    */
    void onMessage(const QString &name, const QString &message);

    /**
    * @brief Обработка прогресса операции
    * @param percent Процент завершения [0; 100]
    * @param stage Название этапа
    */
    void onProgress(int percent, const QString &stage);

    /**
    * @brief Обработка изменения состояния готовности от компонентов
    * @param state Новое состояние FCReadyState
    */
    void onReadyStateChanged(FCReadyState state);

//    /**
//    * @brief Обработка изменения состояния воспроизведения от компонентов
//    * @param state Новое состояние FCPlayState
//    */
//    void onPlayStateChanged(FCPlayState state);

//    /**
//    * @brief Обработка изменения состояния изменений от компонентов
//    * @param state Новое состояние FCChangedState
//    */
//    void onChangedStateChanged(FCChangedState state);

//    /**
//    * @brief Обработка изменения типа ошибки от компонентов
//    * @param type Новый тип ошибки FCErrorType
//    */
//    void onErrorTypeChanged(FCErrorType type);

signals:
    /**
    * @brief Сигнал об изменении состояния готовности приложения
    * @param state Новое состояние FCReadyState
    */
    void readyStateChanged(FCReadyState state);

    /**
    * @brief Сигнал об изменении состояния воспроизведения приложения
    * @param state Новое состояние FCPlayState
    */
    void playStateChanged(FCPlayState state);

    /**
    * @brief Сигнал об изменении состояния изменений приложения
    * @param state Новое состояние FCChangedState
    */
    void changedStateChanged(FCChangedState state);

    /**
    * @brief Сигнал об изменении типа ошибки приложения
    * @param type Новый тип ошибки FCErrorType
    */
    void errorTypeChanged(FCErrorType type);

    void panelStateChanged(FCPanelState state);

    void visibilityStateChanged(FCVisibilityState state);

private:
    // ЧЛЕНЫ ДАННЫХ
    /// @brief Централизованное хранилище состояний класса
    FCMachineState _state;

    /// @brief Главный дисплей для управления и диагностики
    FCDisplay _display;

    /// @brief Парсер SVG файлов
    FCSVGImageParser _parser;

    /// @brief Список плоттеров
    FCPlotterPtrList _plotterList;

    /// @brief Флаг успешной инициализации
    bool _initialized = false;
};

#endif // FC_MACHINE_H
