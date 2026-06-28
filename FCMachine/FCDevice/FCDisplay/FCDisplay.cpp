// FCDisplay.cpp
// Реализация главного окна графического интерфейса системы управления плоттером
// Версия: 2.0.0
// Архитектура: Управление состояниями через шаблонный класс FCStateT<>

#include <QTouchEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDateTime>
#include <QDir>
#include <QScroller>
#include <QScrollerProperties>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDebug>
#include <QCheckBox>

#include "FCConfigFile.h"
#include "FCErrorDialog.h"
#include "FCDisplay.h"
#include "ui_FCDisplay.h"

// КОНСТРУКТОР / ДЕСТРУКТОР
FCDisplay::FCDisplay(QWidget *parent)
    : QMainWindow(parent),
  _ui{new Ui::FCDisplay},
//  FCReadyState, FCPlayState, FCErrorType, FCPanelState, FCVisibilityState
  _state{FCReadyState::NotReady, FCPlayState::Stop, FCErrorType::None, FCPanelState::Main, FCVisibilityState::Show},
  _screenSaverTimer{new QTimer(this)},
  _watcher{new QFileSystemWatcher(this)},
  _touchStartPos{},
  _isTouching{false},
  _modelsBuilt{false},
  _plottersBuilt{false}
{
    _ui->setupUi(this);

    init();
}

FCDisplay::~FCDisplay()
{
    delete _ui;
}

// МЕТОДЫ УСТАНОВКИ СОСТОЯНИЙ (с эмиссией сигналов)
void FCDisplay::set(FCReadyState state)
{
    if(_state.is<FCReadyState>(state) == false)
    {
        _state.set<FCReadyState>(state);
        emit readyStateChanged(state);  // Эмитим сигнал здесь
    }
}

void FCDisplay::set(FCPlayState state)
{
    if(_state.is<FCPlayState>(state) == false)
    {
        _state.set<FCPlayState>(state);
        emit playStateChanged(state);
        update(state);
    }
}

//void FCDisplay::set(FCChangedState state)
//{
//    if(_state.is<FCChangedState>(state) == false)
//    {
//        _state.set<FCChangedState>(state);
//        emit changedStateChanged(state);
//    }
//}

void FCDisplay::set(FCErrorType type)
{
    if(_state.is<FCErrorType>(type) == false)
    {
        _state.set<FCErrorType>(type);
        emit errorTypeChanged(type);
    }
}

void FCDisplay::set(FCPanelState panel)
{
    if(_state.is<FCPanelState>(panel) == false)
    {
        _state.set<FCPanelState>(panel);
        emit panelStateChanged(panel);
        update(panel);
    }
}

void FCDisplay::set(FCVisibilityState visibility)
{
    if(_state.is<FCVisibilityState>(visibility) == false)
    {
        _state.set<FCVisibilityState>(visibility);
        emit visibilityStateChanged(visibility);
    }
}

// ФИЛЬТР СОБЫТИЙ (для перехвата касаний от дочерних виджетов)
bool FCDisplay::eventFilter(QObject *obj, QEvent *event)
{
    // Обрабатываем только события касания
    if(event->type() == QEvent::TouchBegin ||
       event->type() == QEvent::TouchEnd ||
       event->type() == QEvent::TouchUpdate ||
       event->type() == QEvent::TouchCancel ||
       event->type() == QEvent::MouseButtonPress ||
       event->type() == QEvent::MouseButtonRelease)
    {
        // Пропускаем событие в основной event() метод
        return QMainWindow::event(event);
    }
    return QMainWindow::eventFilter(obj, event);
}

// ОБРАБОТКА СОБЫТИЙ (свайпы, тач, скринсейвер)
bool FCDisplay::event(QEvent *event)
{
    // === 1. ОБРАБОТКА СВАЙПА: НАЧАЛО КАСАНИЯ ===
    if(event->type() == QEvent::TouchBegin || event->type() == QEvent::MouseButtonPress)
    {
        if(event->type() == QEvent::TouchBegin)
        {
            auto *touchEvent = static_cast<QTouchEvent*>(event);
            if(!touchEvent->touchPoints().isEmpty())
            {
                _touchStartPos = touchEvent->touchPoints().first().pos().toPoint();
                _isTouching = true;
            }
        }
        else
        {
            auto *mouseEvent = static_cast<QMouseEvent*>(event);
            _touchStartPos = mouseEvent->pos();
            _isTouching = true;
        }
    }
    // === 2. ОБРАБОТКА СВАЙПА: ЗАВЕРШЕНИЕ КАСАНИЯ ===
    else if((event->type() == QEvent::TouchEnd || event->type() == QEvent::MouseButtonRelease) && _isTouching)
    {
        QPoint touchEndPos;
        bool validPos = false;

        if(event->type() == QEvent::TouchEnd)
        {
            auto *touchEvent = static_cast<QTouchEvent*>(event);
            if(!touchEvent->touchPoints().isEmpty())
            {
                touchEndPos = touchEvent->touchPoints().first().pos().toPoint();
                validPos = true;
            }
        }
        else
        {
            auto *mouseEvent = static_cast<QMouseEvent*>(event);
            touchEndPos = mouseEvent->pos();
            validPos = true;
        }

        if(validPos)
        {
            int deltaX = touchEndPos.x() - _touchStartPos.x();
            int deltaY = touchEndPos.y() - _touchStartPos.y();

            // Горизонтальный свайп → переключение панелей
            if(qAbs(deltaX) > qAbs(deltaY) && qAbs(deltaX) > 50)
            {
                if(is(FCPanelState::ScreenSaver))
                {
                    qDebug() << ">>> SWIPE: Exit ScreenSaver";
                    onSwitchToPanel(_previousPanel);
                    _screenSaverTimer->start();
                    _isTouching = false;
                    return true;
                }
                else
                {
                    if(deltaX > 0)
                    {
                        qDebug() << ">>> SWIPE RIGHT: to Main";
                        onSwitchToPanel(FCPanelState::Main);
                    }
                    else
                    {
                        qDebug() << ">>> SWIPE LEFT: to Statistics";
                        onSwitchToPanel(FCPanelState::Statistics);
                    }
                    _isTouching = false;
                    return true;
                }
            }
        }
        _isTouching = false;
    }
    else if(event->type() == QEvent::TouchCancel)
    {
        _isTouching = false;
    }

    // === 3. ВЫХОД ИЗ СКРИНСЕЙВЕРА ПО ЛЮБОМУ КАСАНИЮ ===
    if(event->type() == QEvent::MouseButtonPress ||
       event->type() == QEvent::MouseButtonRelease ||
       event->type() == QEvent::MouseMove ||
       event->type() == QEvent::TouchBegin ||
       event->type() == QEvent::TouchUpdate ||
       event->type() == QEvent::TouchEnd ||
       event->type() == QEvent::KeyPress)
    {
        // Сброс таймера заставки
        if(_screenSaverTimer->isActive() && !is(FCPanelState::ScreenSaver))
        {
            _screenSaverTimer->start();
        }

        // Выход из заставки по любому касанию/клику
        if(is(FCPanelState::ScreenSaver) &&
           (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::TouchBegin ||
            event->type() == QEvent::MouseMove))
        {
            qDebug() << ">>> TOUCH: Exit ScreenSaver";
            onSwitchToPanel(_previousPanel);
            _screenSaverTimer->start();
            return true;
        }
    }

    return QMainWindow::event(event);
}

// ОБНОВЛЕНИЕ ИНТЕРФЕЙСА (ПАНЕЛИ)
void FCDisplay::update(FCPanelState panel)
{
    // Если панель уже активна — обновляем только состояние виджетов с учетом PlayState
    if(_ui->stackedWidget->currentIndex() == static_cast<int>(panel))
    {
        _ui->model->setEnabled((panel == FCPanelState::Main) &&
                               !is(FCPlayState::Start) &&
                               !is(FCPlayState::Pause) &&
                               !_ui->model->objectName().isEmpty());
        _ui->models->setEnabled((panel == FCPanelState::Main) &&
                                !is(FCPlayState::Start) &&
                                !is(FCPlayState::Pause));
        _ui->playPause->setEnabled((panel == FCPanelState::Main));
        _ui->stop->setEnabled((panel == FCPanelState::Main) &&
                              (is(FCPlayState::Start) || is(FCPlayState::Pause)));
        _ui->changePlotter->setEnabled((panel == FCPanelState::Main) &&
                                       !is(FCPlayState::Start) &&
                                       !is(FCPlayState::Pause) &&
                                       _ui->plotters->count() > 1);
        _ui->ssButton->setEnabled(panel == FCPanelState::ScreenSaver);
        return;  // Выходим рано, избегаем лишней перерисовки
    }

    // Переключаем панель только если индекс изменился
    _ui->stackedWidget->setCurrentIndex(static_cast<int>(panel));

    // Применяем состояние с учетом PlayState сразу после переключения
    _ui->model->setEnabled((panel == FCPanelState::Main) &&
                           !is(FCPlayState::Start) &&
                           !is(FCPlayState::Pause) &&
                           !_ui->model->objectName().isEmpty());
    _ui->models->setEnabled((panel == FCPanelState::Main) &&
                            !is(FCPlayState::Start) &&
                            !is(FCPlayState::Pause));
    _ui->playPause->setEnabled((panel == FCPanelState::Main));
    _ui->stop->setEnabled((panel == FCPanelState::Main) &&
                          (is(FCPlayState::Start) || is(FCPlayState::Pause)));
    _ui->changePlotter->setEnabled((panel == FCPanelState::Main) &&
                                   !is(FCPlayState::Start) &&
                                   !is(FCPlayState::Pause) &&
                                   _ui->plotters->count() > 1);
    _ui->ssButton->setEnabled(panel == FCPanelState::ScreenSaver);

    // Управление таймером заставки
    if(panel == FCPanelState::ScreenSaver)
    {
        _screenSaverTimer->stop();
    }
    else if(!_screenSaverTimer->isActive())
    {
        _screenSaverTimer->start();
    }
}

// ОБНОВЛЕНИЕ ИНТЕРФЕЙСА (ВОСПРОИЗВЕДЕНИЕ)
void FCDisplay::update(FCPlayState state)
{
    // При изменении состояния PlayState нужно также обновить доступность кнопок на текущей панели
    // Получаем текущую панель
    FCPanelState currentPanel = static_cast<FCPanelState>(_ui->stackedWidget->currentIndex());

    // Обновляем иконки и специфичные состояния плеера
    switch(state)
    {
        case FCPlayState::Stop:
            _ui->playPause->setIcon(QIcon(DefaultPlayImage));
            _ui->stop->setEnabled(false);
            // При остановке возвращаем возможность выбора модели, если мы на главной панели
            if(currentPanel == FCPanelState::Main)
            {
                _ui->model->setEnabled(!_ui->model->objectName().isEmpty());
                _ui->models->setEnabled(true);
                _ui->changePlotter->setEnabled(_ui->plotters->count() > 1);
            }
        break;

        case FCPlayState::Start:
            _ui->playPause->setIcon(QIcon(DefaultPauseImage));
            _ui->stop->setEnabled(true);
            // Блокируем выбор модели и списка во время воспроизведения
            _ui->model->setEnabled(false);
            _ui->models->setEnabled(false);
            _ui->changePlotter->setEnabled(false);
            break;

        case FCPlayState::Pause:
            _ui->playPause->setIcon(QIcon(DefaultPlayImage));
            _ui->stop->setEnabled(true);
            // В паузе выбор все еще заблокирован (логика может быть изменена по требованию)
            _ui->model->setEnabled(false);
            _ui->models->setEnabled(false);
            _ui->changePlotter->setEnabled(false);
            break;
    }
    _ui->playPause->repaint();

    // Дополнительно обновляем общую структуру интерфейса на случай, если мы находимся в скринсейвере или другой панели
    // Это гарантирует синхронизацию при переключении состояний извне
    update(currentPanel);
}

// УСТАНОВКА ФИЛЬТРОВ НА ДОЧЕРНИЕ ВИДЖЕТЫ
void FCDisplay::installTouchFilters()
{
    // Устанавливаем фильтр событий на все виджеты, которые могут перехватывать касания
    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    for(QWidget *widget : widgets)
    {
        // Пропускаем кнопки и элементы управления, которые должны работать
        if(qobject_cast<QPushButton*>(widget) ||
           qobject_cast<QToolButton*>(widget) ||
           qobject_cast<QCheckBox*>(widget))
        {
            continue;
        }
        // Устанавливаем фильтр на остальные виджеты
        widget->installEventFilter(this);
        // Разрешаем propagate события касания
        widget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    }
}

// ОСНОВНАЯ ИНИЦИАЛИЗАЦИЯ
void FCDisplay::init()
{
    // Инициализация флагов кэширования
    _modelsBuilt = false;
    _plottersBuilt = false;

    buildPlottersMenu();  // Построить один раз
    buildModelsMenu();    // Построить один раз
    buildDialogs();

    setAttribute(Qt::WA_AcceptTouchEvents, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    _ui->console->hide();
    _ui->progressBar->hide();

    // Начальное состояние кнопок
    // PlayPause
    _ui->playPause->setEnabled(!_ui->model->objectName().isEmpty() && is(FCPlayState::Stop));
    connect(_ui->playPause, &QPushButton::clicked, this, &FCDisplay::onPlayPausePressed);

    // Model
    _ui->model->setEnabled(!_ui->model->objectName().isEmpty() && is(FCPlayState::Stop));
    connect(_ui->model, &QToolButton::clicked, this, &FCDisplay::onPlayPausePressed);

    // ModelStatistic
//    _ui->modelStatistic->setEnabled(!_ui->modelStatistic->objectName().isEmpty() && is(FCPlayState::Stop));

    // Stop
    _ui->stop->setEnabled(!is(FCPlayState::Stop));
    connect(_ui->stop, &QPushButton::clicked, this, [this]() { _yn.show(); });

    // ChangePlotter
//    _ui->changePlotter->setEnabled(_ui->plotters->count() <= 1 ? false : true);
    _ui->changePlotter->setEnabled(false);
    connect(_ui->changePlotter, &QPushButton::clicked, this, &FCDisplay::onChangePlotterPressed);

    // ScreenSaverButton
    _ui->ssButton->setEnabled(false);
    _screenSaverTimer->setInterval(DefaultScreenSaverTimeOut);
    _screenSaverTimer->start();
    connect(_screenSaverTimer, &QTimer::timeout, this, [this](){ onSwitchToPanel(FCPanelState::ScreenSaver); });

    connect(&_error, &FCErrorDialog::ok, &_error, &QDialog::hide);
    connect(_watcher, &QFileSystemWatcher::directoryChanged, this, &FCDisplay::onDirectoryChanged);
    connect(_ui->ssButton, &QToolButton::clicked, this, [this](){ onSwitchToPanel(_previousPanel); });

    // Plotters ?
    connect(_ui->plotters, &QTabWidget::currentChanged, this, [](int index) { Q_UNUSED(index); });
//    connect(_ui->plotters->tabBar(), &QTabBar::tabBarDoubleClicked, this, &FCDisplay::onConsole);

    // StatusLine
    _ui->statusLine->setText(" > ");


    // ЯВНАЯ ИНИЦИАЛИЗАЦИЯ _previousPanel
    _previousPanel = FCPanelState::Main;
    onSwitchToPanel(FCPanelState::Main);
    onModelDataShow(QFileInfo());

    // Wather
    _watcher->addPath(FCConfigFile::instance().modelsPath());

    // УСТАНОВКА ФИЛЬТРОВ НА ДОЧЕРНИЕ ВИДЖЕТЫ
    installTouchFilters();
}

// ДИАЛОГИ
void FCDisplay::buildDialogs()
{
    _yn.setMessage("Вы пытаетесь прервать\nпроцесс вывода изображения!\nЕсли вы уверены,\nподтвердите ваше решение.");
    connect(&_yn, &FCYesNoDialog::yes, this, [this]()
    {
        _yn.hide();
        _ui->model->setEnabled(true);
        _ui->models->setEnabled(true);
        _ui->stop->setEnabled(false);
        _ui->playPause->setIcon(QIcon(DefaultPlayImage));
        set(FCPlayState::Stop);
        emit stop();
    });
    connect(&_yn, &FCYesNoDialog::no, this, [this]()
    {
        _yn.hide();
        _ui->stop->setEnabled(true);
    });

    _error.hide();
}

// МЕНЮ ПЛОТТЕРОВ
void FCDisplay::buildPlottersMenu()
{
    // ПРОВЕРКА: не перестраивать если уже построено
    if(_plottersBuilt && _ui->plotters->count() > 0)
    {
        qDebug() << ">>> Plotters already built, skipping...";
        return;
    }
    while(_ui->plotters->count() > 0)
    {
        QWidget *widget = _ui->plotters->widget(0);
        _ui->plotters->removeTab(0);
        delete widget;
    }
    const QStringList &serialNumbers = FCConfigFile::instance().plottersSerialNumbers();

    if(serialNumbers.isEmpty())
    {
        QWidget *placeholder = new QWidget();
        _ui->plotters->addTab(placeholder, "Нет плоттеров");
        _ui->plotters->setTabEnabled(0, false);
    }
    else
    {
        for(int i = 0; i < serialNumbers.size(); ++i)
        {
            const QString &serial = serialNumbers.at(i);
            QString tabName = "~ " + serial.right(4);
            QWidget *widget = new QWidget();
            _ui->plotters->addTab(widget, tabName);
        }
        _ui->plotters->setCurrentIndex(0);
    }

    // Установить флаг после успешной постройки
    _plottersBuilt = true;
}

// МЕНЮ МОДЕЛЕЙ
void FCDisplay::buildModelsMenu()
{
    //  ПРОВЕРКА: не перестраивать если уже построено
    if(_modelsBuilt)
    {
        qDebug() << ">>> Models already built, skipping...";
        return;
    }
    QLayoutItem *item;
    while((item = _gridOfModels.takeAt(0)) != nullptr)
    {
        if(item->widget())
        {
            delete item->widget();
        }
        delete item;
    }

//    QDir dir(FCConfigFile::instance().modelsPath());
//    if(!dir.exists())
//    {
//        showNoModelsPlaceHolder();
//        _modelsBuilt = true;
//        return;
//    }

    QFileInfoList modelsList{QDir(FCConfigFile::instance().modelsPath()).entryInfoList({"*" + FCConfigFile::instance().modelExtension()}, QDir::Files | QDir::Readable, QDir::Name)};

    if(modelsList.isEmpty())
    {
        showNoModelsPlaceHolder();
        _modelsBuilt = true;
        return;
    }

    _gridOfModels.setVerticalSpacing(static_cast<int>(DefaultModelSpacing.height()));
    _gridOfModels.setHorizontalSpacing(static_cast<int>(DefaultModelSpacing.width()));

    // === ВЕРТИКАЛЬНАЯ ПРОКРУТКА: минимальная высота для скролла ===
    const int buttonHeight = static_cast<int>(DefaultModelButtonSize.height());
    const int vSpacing = static_cast<int>(DefaultModelSpacing.height());
    const int rows = (modelsList.size() + 6) / 7;
    const int minGridHeight = rows * buttonHeight + (rows > 1 ? (rows - 1) * vSpacing : 0);
    _ui->scrollAreaWidgetContents->setMinimumHeight(minGridHeight);
    _ui->scrollAreaWidgetContents->setMinimumWidth(0);

    int row = 0;
    int column = 0;
    const int maxColumns = 7;

    for(auto &model : modelsList)
    {
        if(column >= maxColumns)
        {
            row++;
            column = 0;
        }

        QToolButton *modelButton = new QToolButton(_ui->models->widget());
        modelButton->setIconSize(QSize(DefaultModelIconSize.width(), DefaultModelIconSize.height()));
        modelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        QIcon icon(model.absoluteFilePath());
        if(icon.isNull())
        {
            icon = QIcon(DefaultFNFImage);
        }

        modelButton->setObjectName(model.baseName());
        modelButton->setIcon(icon);
        modelButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

        connect(modelButton, &QToolButton::clicked, this, &FCDisplay::onSelectModelsElement);

        _gridOfModels.addWidget(modelButton, row, column);
        column++;
    }

    if(_ui->scrollAreaWidgetContents->layout() == nullptr)
    {
        // === ВЕРТИКАЛЬНЫЙ СКРОЛЛ + QScroller для тач-свайпа ===
        _ui->models->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _ui->models->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _ui->models->setWidgetResizable(true);
        _ui->models->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);

        // Включаем QScroller для вертикального свайпа
        QScroller::grabGesture(_ui->models->viewport(), QScroller::LeftMouseButtonGesture);

        QScrollerProperties scrollerProperties;
        scrollerProperties.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
        scrollerProperties.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
        scrollerProperties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
        scrollerProperties.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.5);

        QScroller *scroller = QScroller::scroller(_ui->models->viewport());
        scroller->setScrollerProperties(scrollerProperties);

        _ui->scrollAreaWidgetContents->setLayout(&_gridOfModels);
    }

    // Установить флаг после успешной постройки
    _modelsBuilt = true;
}

// ЗАГЛУШКА ПРИ ОТСУТСТВИИ МОДЕЛЕЙ
void FCDisplay::showNoModelsPlaceHolder()
{
    QToolButton *placeholder = new QToolButton(_ui->models->widget());
    placeholder->setIcon(QIcon(DefaultFNFImage));
    placeholder->setFixedSize(QSize(static_cast<int>(DefaultModelButtonSize.width()*3), static_cast<int>(DefaultModelButtonSize.height()*3)));
    placeholder->setIconSize(QSize(static_cast<int>(DefaultModelIconSize.width()*3), static_cast<int>(DefaultModelIconSize.height()*3)));
    placeholder->setText("Модели\nне найдены");
    qDebug() << "objectName() = " << placeholder->objectName();
    placeholder->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    placeholder->setEnabled(false);
    _gridOfModels.setAlignment(Qt::AlignCenter);
    _gridOfModels.addWidget(placeholder, 0, 0);
    if(_ui->scrollAreaWidgetContents->layout() == nullptr)
    {
        _ui->models->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _ui->models->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        _ui->scrollAreaWidgetContents->setLayout(&_gridOfModels);
    }
}

// КОНСОЛЬ ЛОГОВ
void FCDisplay::onConsole()
{
    if(is(FCVisibilityState::Show))
    {
        _ui->console->setGeometry(DefaultConsoleSize.first);
        _ui->progressBar->setGeometry(DefaultProgressBarSize.first);
        _ui->console->show();
    }
    else
    {
        _ui->console->setGeometry(DefaultConsoleSize.second);
        _ui->progressBar->setGeometry(DefaultProgressBarSize.second);
        _ui->console->hide();
    }
    _ui->console->repaint();
    _ui->progressBar->repaint();
    _screenSaverTimer->start();
}

// ВЫБОР МОДЕЛИ
void FCDisplay::onSelectModelsElement()
{
    if(!_ui->console->isHidden())
    {
        onConsole();
    }

    QObject *pSender = sender();
    if(!pSender)
    {
        return;
    }

    QToolButton *pButton = qobject_cast<QToolButton*>(pSender);
    if(!pButton)
    {
        return;
    }

    QString modelName = pButton->objectName();
    if(modelName.isEmpty())
    {
        return;
    }

    QString modelPath = FCConfigFile::instance().modelsPath() + QDir::separator() + modelName + FCConfigFile::instance().modelExtension();

    QIcon modelIcon(modelPath);
    if(modelIcon.isNull())
    {
        modelIcon = QIcon(DefaultFNFImage);
    }

    _ui->model->setIcon(modelIcon);
    _ui->model->setObjectName(modelName);
    _ui->model->repaint();

    onModelDataShow(QFileInfo(modelPath));

    _ui->model->setEnabled(true);
    _ui->playPause->setEnabled(true);

    set(FCPlayState::Stop);

    _screenSaverTimer->start();
}

// МЕТАДАННЫЕ МОДЕЛИ
void FCDisplay::onModelDataShow(const QFileInfo &fileInfo)
{
    if(!fileInfo.exists())
    {
        _ui->fileData->setText("Модель не выбрана");
        return;
    }
    QString sizeStr;
    qint64 size = fileInfo.size();
    if(size < 1024)
    {
        sizeStr = QString("%1 байт").arg(size);
    }
    else if(size < 1024 * 1024)
    {
        sizeStr = QString("%1 КБ").arg(size / 1024.0, 0, 'f', 1);
    }
    else
    {
        sizeStr = QString("%1 МБ").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
    }
    _ui->fileData->setText(QString(
        "Имя модели: %1\n"
        "Дата изменения: %2\n"
        "Размер: %3"
    ).arg(
        fileInfo.fileName(),
        fileInfo.lastModified().toString("dd MMMM yyyy"),
        sizeStr
    ));
}

void FCDisplay::onStatusLineOut(const QString &name, const QString &statusMessage)
{
    _ui->statusLine->setText(" > " + name + ": " + statusMessage);
    _ui->statusLine->repaint();
}

void FCDisplay::onDirectoryChanged(const QString &dir)
{
    qDebug() << "1. icon().name() = " << _ui->model->icon().name() <<  ", DefaultFNFImage = " << DefaultFNFImage;
    if(_ui->model->icon().name() == DefaultFNFImage)
    {
        qDebug() << "2. objectName = " << _ui->model->objectName();
        rebuildModels();
        return;
    }

    // Получаем актуальный список файлов
    QFileInfoList files{QDir(dir).entryInfoList({"*" + FCConfigFile::instance().modelExtension()}, QDir::Files | QDir::Readable, QDir::Name)};

    bool found = false;
    for(const QFileInfo &file : files)
    {
        if(file.fileName() == _ui->model->objectName())
        {
            qDebug() << QString("3. fileName() = %1 objectName() = %2").arg(file.fileName()).arg(_ui->model->objectName());
            found = true;
            break;
        }
    }

    if(!found)
    {
        onModelDataShow(QFileInfo());
        // тут нужно синхронизировать список моделей и имеющиеся файлы вместо полной перестройки
        rebuildModels();
    }
}

// ВОСПРОИЗВЕДЕНИЕ (PLAY/PAUSE)
void FCDisplay::onPlayPausePressed()
{
    _screenSaverTimer->start();
    if(is(FCPlayState::Stop) || is(FCPlayState::Pause))
    {
        auto modelName = _ui->model->objectName();
        if(modelName.isEmpty() || modelName == DefaultFNFImage)
        {
            _error.setMessage("Не выбрана модель для воспроизведения");
            _error.show();
            return;
        }

        emit play(modelName);

        _ui->playPause->setIcon(QIcon(DefaultPauseImage));
        set(FCPlayState::Start);

        // Состояния кнопок обновятся автоматически через вызов set(FCPlayState::Start) внутри update(FCPlayState)
        // Но для надежности можно явно вызвать update текущей панели, если логика разнесена
        update(static_cast<FCPanelState>(_ui->stackedWidget->currentIndex()));
    }
    else if(is(FCPlayState::Start))
    {
        emit pause();

        _ui->playPause->setIcon(QIcon(DefaultPlayImage));
        set(FCPlayState::Pause);

        update(static_cast<FCPanelState>(_ui->stackedWidget->currentIndex()));
    }

    _ui->playPause->repaint();
}

// ПРОГРЕСС ПАРСИНГА
void FCDisplay::onProgress(int percent)
{
    _screenSaverTimer->start();
    if(percent > 0 && !_ui->progressBar->isVisible())
    {
        _ui->progressBar->show();
    }

    _ui->progressBar->setValue(percent);

    if(percent >= 100)
    {
        QTimer::singleShot(500, this, [this]()
        {
            if(_ui->progressBar->value() >= 100)
            {
                _ui->progressBar->hide();
            }
        });
    }
}

// ПЕРЕКЛЮЧЕНИЕ ПЛОТТЕРА
void FCDisplay::onChangePlotterPressed()
{
    _screenSaverTimer->start();
    int currentIndex = _ui->plotters->currentIndex();
    int plottersCount = _ui->plotters->count();
    if(plottersCount <= 1)
    {
        return;
    }

    int nextIndex = (currentIndex + 1) % plottersCount;
    _ui->plotters->setCurrentIndex(nextIndex);
}

// ПЕРЕКЛЮЧЕНИЕ ПАНЕЛИ
void FCDisplay::onSwitchToPanel(const FCPanelState panel)
{
    if(panel != FCPanelState::ScreenSaver)
    {
        _screenSaverTimer->start();
        _previousPanel = panel;
    }
    if(_ui->stackedWidget->currentIndex() != static_cast<int>(panel))
    {
        _ui->stackedWidget->setCurrentIndex(static_cast<int>(panel));
        set(panel);
    }
    else
    {
        // Даже если панель не меняется (например, повторный вызов),
        // нужно убедиться, что состояния кнопок корректны (особенно после скринсейвера)
        set(panel);
    }
}

// ЛОГИРОВАНИЕ СООБЩЕНИЙ
void FCDisplay::onMessage(const QString &name, const QString &message)
{
    _screenSaverTimer->start();
    QTextEdit *textEdit = qobject_cast<QTextEdit*>(_ui->console);
    QPlainTextEdit *plainEdit = qobject_cast<QPlainTextEdit*>(_ui->console);
    if(!textEdit && !plainEdit)
    {
        return;
    }

    if(!_ui->console->isVisible())
    {
        onConsole();
    }

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString formattedMessage = QString("[%1] %2: %3\n").arg(timestamp, name, message);

    const int maxBufferSize = 10000;

    if(textEdit)
    {
        if(textEdit->toPlainText().size() > maxBufferSize)
        {
            textEdit->clear();
        }
        textEdit->append(formattedMessage);
        textEdit->ensureCursorVisible();
    }
    else if(plainEdit)
    {
        if(plainEdit->toPlainText().size() > maxBufferSize)
        {
            plainEdit->clear();
        }
        plainEdit->appendPlainText(formattedMessage);
        plainEdit->ensureCursorVisible();
    }
}

// ДОБАВЛЕНИЕ ПЛОТТЕРА
void FCDisplay::addPlotterName(const QString &name)
{
    _screenSaverTimer->start();
    QStringList serialNumbers = FCConfigFile::instance().plottersSerialNumbers();
    if(!serialNumbers.contains(name))
    {
        serialNumbers.append(name);
        // Сбросить флаг и перестроить при обнаружении нового плоттера
        _plottersBuilt = false;
        buildPlottersMenu();
        onMessage("System", QString("Обнаружен новый плоттер: %1").arg(name));
    }
}

// ПЕРЕСТРОЙКА МОДЕЛЕЙ
void FCDisplay::rebuildModels()
{
    _modelsBuilt = false;
    buildModelsMenu();
}
