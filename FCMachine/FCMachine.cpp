// FCMachine.cpp
// Реализация главного класса приложения FCMachine
// Версия: 2.0.0
// Архитектура: Управление состояниями через класс FCState

#include <QTextStream>
#include <QCommandLineParser>
#include <QDebug>

#include "FCMachine.h"

/// КОНСТРУКТОР / ДЕСТРУКТОР
FCMachine::FCMachine(int argc, char **argv)
: QApplication(argc, argv),
  _state{FCReadyState::NotReady, FCErrorType::None},
  _display{},
  _parser{this},
  _plotterList{},
  _initialized{false}
{
    setStyleSheet(StyleSheetMachine);
    setOrganizationName("K-Service");
    setOrganizationDomain("www.k-service.ru");
    setApplicationName("Funny Cake Plotter");
    setApplicationVersion("0.17.03.260309"); // 0 - релиз, 17 - подрелиз, 03 - номер подверсии, 26 - год + 05- месяц + 17 - день месяца
    setApplicationDisplayName(applicationName());

    if(cla())
    {
        _initialized = init();

        // Инициализация состояния через _state
        set(FCReadyState::Ready);
    }
}

// ГЛАВНЫЙ ЦИКЛ ПРИЛОЖЕНИЯ
int FCMachine::exec()
{
    // Проверяем состояние через методы is() FCState
    if(is(FCReadyState::Ready))
    {
//        qDebug() << "display show ...";
        _display.show();
        return QApplication::exec();
    }

    return -1;
}

// ИНИЦИАЛИЗАЦИЯ
bool FCMachine::init()
{
    // СВЯЗИ: FCDisplay ↔ FCSVGImageParser
    connect(&_display, &FCDisplay::play, &_parser, &FCSVGImageParser::parseAsync);

    // ОТЛАДКА (временные заглушки)
    connect(&_display, &FCDisplay::play, this, [](const QString &model) { qDebug() << QString("SELECTED %1 & PLAY PRESSED ...").arg(model); });
    connect(&_display, &FCDisplay::pause, this, []() { qDebug() << "PAUSE PRESSED ..."; });
    connect(&_display, &FCDisplay::stop, this, []() { qDebug() << "STOP PRESSED ..."; });

    // СВЯЗИ: FCSVGImageParser ↔ FCMachine
    connect(&_parser, &FCSVGImageParser::finished, this, &FCMachine::onParsingFinished);
    connect(&_parser, &FCSVGImageParser::progress, this, &FCMachine::onProgress);
    connect(&_parser, &FCSVGImageParser::error, &_display, &FCDisplay::onMessage);

    // Подключение сигналов состояний от парсера
    connect(&_parser, &FCSVGImageParser::readyStateChanged, this, &FCMachine::onReadyStateChanged);
//    connect(&_parser, &FCSVGImageParser::playStateChanged, this, &FCMachine::onReadyStateChanged);
//    connect(&_parser, &FCSVGImageParser::changedStateChanged, this, &FCMachine::onChangedStateChanged);
//    connect(&_parser, &FCSVGImageParser::errorTypeChanged, this, &FCMachine::onErrorTypeChanged);

    // СВЯЗИ: FCDisplay ↔ FCMachine (синхронизация состояний через FCState)
    connect(&_display, &FCDisplay::readyStateChanged, this, &FCMachine::onReadyStateChanged);
//    connect(&_display, &FCDisplay::playStateChanged, this, &FCMachine::onPlayStateChanged);
//    connect(&_display, &FCDisplay::changedStateChanged, this, &FCMachine::onChangedStateChanged);
//    connect(&_display, &FCDisplay::errorTypeChanged, this, &FCMachine::onErrorTypeChanged);

    // ЗАПУСК РАБОЧИХ ПОТОКОВ (закомментировано для будущей реализации)
    //    if(!_coder.startThread()) { ... }
    //    if(!_plotter.startThread()) { ... }

    return true;
}

// ОБРАБОТКА ИЗМЕНЕНИЙ СОСТОЯНИЙ
void FCMachine::onReadyStateChanged(FCReadyState state)
{
    // Синхронизируем локальное состояние с состоянием компонента
    set(state);
    qDebug() << ">>> FCMachine: ReadyState changed to" << static_cast<int>(state);
}

//void FCMachine::onPlayStateChanged(FCPlayState state)
//{
//    // Синхронизируем локальное состояние с состоянием компонента
//    set(state);
//    qDebug() << ">>> FCMachine: PlayState changed to" << static_cast<int>(state);
//}

//void FCMachine::onChangedStateChanged(FCChangedState state)
//{
//    // Синхронизируем локальное состояние с состоянием компонента
//    set(state);
//    qDebug() << ">>> FCMachine: ChangedState changed to" << static_cast<int>(state);
//}

//void FCMachine::onErrorTypeChanged(FCErrorType type)
//{
//    // Синхронизируем локальное состояние с состоянием компонента
//    set(type);
//    qDebug() << ">>> FCMachine: ErrorType changed to" << static_cast<int>(type);
//}

// ОБРАБОТКА ЗАВЕРШЕНИЯ ПАРСИНГА
void FCMachine::onParsingFinished(const FCSVGImageParser::ParseResult &result)
{
    if(result.success)
    {
        // Устанавливаем состояние готовности после успешного парсинга
        _display.onMessage(QStringLiteral("Parser"),
                           QStringLiteral("Parsing is finished: %1 фигур, %2 точек").arg(result.figuresParsed).arg(result.pointsTotal));
        set(FCReadyState::Ready);
    }
    else
    {
        _display.onMessage(QStringLiteral("Parser"),
                           QStringLiteral("Parsing error: %1").arg(result.errorMessage));
        // Устанавливаем критическую ошибку
        set(FCReadyState::NotReady);
        set(FCErrorType::Critical);
    }
}

// ОБРАБОТКА СООБЩЕНИЙ И ПРОГРЕССА
void FCMachine::onMessage(const QString &name, const QString &message)
{
    _display.onMessage(name, message);
}

void FCMachine::onProgress(int percent, const QString &stage)
{
    Q_UNUSED(stage);
    // В будущем может быть добавить вывод сообщения stage
    _display.onParsingProcess(percent);
}

// КОМАНДНАЯ СТРОКА
bool FCMachine::cla()
{
    QCommandLineParser parser;
    QString msg[] { "About application:  ",  "Configuration file path:  ",  "Models path:  ",  "Codes path:  "};
    parser.setApplicationDescription("Funny Cake Plotter ");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({{{"about", "a"}, msg[0]},
                       {{"config", "c"}, msg[1]},
                       {{"models", "m"}, msg[2]},
                       {{"codes", "d"}, msg[3]}}
    );
    parser.process(*this);
    QTextStream out(stdout);

    if(parser.isSet("about"))
    {
        out << about();
        return false;  // Завершить после вывода
    }

    if(parser.isSet("config"))
    {
        out << msg[1] + FCConfigFile::instance().confPath() + "\n";
        return false;  // Завершить после вывода
    }

    if(parser.isSet("models"))
    {
        out << msg[2] + FCConfigFile::instance().modelsPath() + "\n";
        return false;  // Завершить после вывода
    }

    if(parser.isSet("codes"))
    {
        out << msg[3] + FCConfigFile::instance().codesPath() + "\n";
        return false;  // Завершить после вывода
    }

    return true;
}

// ИНФОРМАЦИЯ О ПРИЛОЖЕНИИ
const QString FCMachine::about()
{
    QString information;
    information += tr("=== FunnyCake Plotter ===\n");
    information += tr("Creator:  ") + organizationName() + "\n";
    information += tr("Website:  ") + organizationDomain() + "\n";
    information += tr("Name:  ") + applicationName() + "\n";
    information += tr("Version:  ") + applicationVersion() + "\n";
    //    info += tr("Build Date:  ") +  DATE  + "\n";
    //    info += tr("Qt Version:  ") + QString::fromUtf8(qVersion()) + "\n";
    return information;
}
