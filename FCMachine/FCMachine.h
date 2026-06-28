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
#include "FCImageBinaryContainer.h"
#include "FCScaner.h"

/////// @brief Набор состояний для машины
using FCMachineState = FCStateT<FCReadyState, FCPlayState, FCErrorType>;

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
    : public QApplication,
      public FCMachineState
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

private:
    FCMachineState& state() { return _state; }

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

signals:
    /**
    * @brief Сигнал об изменении состояния готовности приложения
    * @param state Новое состояние FCMachineState
    */
    void condition(FCMachineState &state);

private:
    /// @brief шина i2c
    FCI2CBus *_i2c = nullptr;

    /// @brief централизованное хранилище состояний класса
//    FCMachineState _state; // в наследовании

    /// @brief главный дисплей для управления и диагностики
    FCDisplay _display;

    /// @brief парсер SVG файлов
    FCSVGImageParser *_parser = nullptr;

    /// @brief плоттер
    FCPlotter *_plotter = nullptr;

    /// @brief сканер
    FCScaner *_scaner = nullptr;

    /// @brief Флаг успешной инициализации
//    bool _initialized = false;
};

#endif // FC_MACHINE_H
