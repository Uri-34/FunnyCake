# ПОЛНЫЙ КОНТЕКСТ ПРОЕКТА FCPlotter
*Сгенерировано: 2026-03-06 10:04:19*

## 📁 Структура проекта с ПОЛНЫМ содержимым .h файлов

```
└── FCMachine/
    ├── doxygen/
    │   ├── context/
    │   ├── html/
    │   │   └── search/
    │   └── scripts/
    ├── FCConfigFile/
    │   ├── build-FCConfigFile-Desktop_Qt_5_15_2_GCC_64bit-Debug/
    │   │   └── .qtc_clangd/
    │   │       └── .cache/
    │   │           └── clangd/
    │   │               └── index/
    │   └── FCConfigFile.h
    │       └── 📦 class FCConfigFile
    │           └── 🏗️ Конструкторы:
    │               └── FCConfigFile()
    │               └── FCConfigFile()
    │               └── FCConfigFile(const FCConfigFile&)
    │               └── FCConfigFile(const FCConfigFile&)(const FCConfigFile& other)
    │           └── 🗑️ ~FCConfigFile()
    │           └── 📡 Публичные методы:
    │               └── brief Возвращает единственный экземпляр класса(ленивая инициализация)
    │               └── 11 и выше(стандартное поведение
    ///       для статических локальных переменных)
    │               └── static FCConfigFile& instance()
    │               └── brief Деструктор(освобождает ресурсы при завершении программы)
    │               └── за приватности деструктора в классическом Singleton(здесь разрешён для
    ///       совместимости с современными практиками)
    │               └── геттеры конфигурации(только для чтения)
    │               └── const QString& modelExtension() [const noexcept]
    │               └── const QString& codeExtension() [const noexcept]
    │               └── const QString& configExtension() [const noexcept]
    │               └── return Абсолютный путь к директории моделей(например, "/usr/share/funnycake/models")
    │               └── note Путь нормализован(без завершающего слеша)
    │               └── const QString& modelsPath() [const noexcept]
    │               └── return Абсолютный путь к директории кодов(например, "/usr/share/funnycake/codes")
    │               └── note Путь нормализован(без завершающего слеша)
    │               └── const QString& codesPath() [const noexcept]
    │               └── return Абсолютный путь к файлу конфигурации(например, "/etc/funnycake/funnycake.conf")
    │               └── const QString& confPath() [const noexcept]
    │               └── const QStringList& plottersSerialNumbers() [const noexcept]
    │               └── note Все текущие значения(включая серийные номера)
    │               └── сделанные программно и не сохранённые через save()
    │               └── void reload()
    │               └── bool persist()
    │               └── return save()
    │           └── 💾 Публичные члены-данные:
    │               └── return _modelExtension
    │               └── return _codeExtension
    │               └── return _configExtension
    │               └── return _modelsPath
    │               └── return _codesPath
    │               └── return _confPath
    │               └── return _plotterSerialNumbers
    │               └── если сохранение успешно
    ├── FCDisplay/
    │   ├── FCErrorDialog/
    │   │   └── FCErrorDialog.h
    │   │       └── 📦 class FCErrorDialog
    │   │           └── 📌 Наследует: public QDialog
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCErrorDialog(QWidget *parent = nullptr)
    │   │               └── FCErrorDialog()
    │   │           └── 🗑️ ~FCErrorDialog()
    │   │           └── 📡 Публичные методы:
    │   │               └── param parent Родительский виджет(для управления временем жизни и модальностью)
    │   │               └── Если родитель не указан(nullptr)
    │   │               └── note Автоматически освобождает ресурсы интерфейса(через удаление _ui)
    │   │               └── param message Текст сообщения(поддерживает HTML-форматирование для Qt-виджетов)
    │   │               └── note Метод можно вызывать как до вызова exec()
    │   │               └── warning Длинные сообщения(>500 символов)
    │   │               └── void setMessage(const QString &message)
    │   │               └── note Этот метод является стандартным для QDialog и вызывается неявно при exec()
    │   │               └── Рекомендуется использовать именно exec()
    │   │               └── int exec() [override]
    │   │           └── 🔊 Сигналы:
    │   │               └── void ok()
    │   ├── FCYesNoDialog/
    │   │   └── FCYesNoDialog.h
    │   │       └── 📦 class FCYesNoDialog
    │   │           └── 📌 Наследует: public QDialog
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCYesNoDialog(QWidget *parent = nullptr)
    │   │               └── FCYesNoDialog()
    │   │               └── FCYesNoDialog("Удалить файл?")
    │   │           └── 🗑️ ~FCYesNoDialog()
    │   │           └── 📡 Публичные методы:
    │   │               └── param parent Родительский виджет(для управления временем жизни и модальностью)
    │   │               └── Если родитель не указан(nullptr)
    │   │               └── warning Для модального поведения вызывайте exec()
    │   │               └── вместо show()
    │   │               └── note Автоматически освобождает ресурсы интерфейса(через удаление _ui)
    │   │               └── param message Текст запроса(поддерживает HTML-форматирование для Qt-виджетов)
    │   │               └── note Метод можно вызывать как до вызова exec()
    │   │               └── warning Длинные сообщения(>500 символов)
    │   │               └── void setMessage(const QString &message)
    │   │               └──  if(FCYesNoDialog("Удалить файл?")
    │   │               └── endcode
    int exec() [override]
    │   │           └── 🔊 Сигналы:
    │   │               └── void yes()
    │   │               └── void no()
    │   └── FCDisplay.h
    │       └── 📦 class FCDisplay
    │           └── 📌 Наследует: public QMainWindow
    │           └── 🏗️ Конструкторы:
    │               └── FCDisplay(QWidget *parent = nullptr)
    │               └── FCDisplay()
    │           └── 🗑️ ~FCDisplay()
    │           └── 📡 Публичные методы:
    │               └── param parent Родительский виджет(обычно nullptr для главного окна)
    │               └── Создаётся интерфейс через Qt Designer(setupUi)
    │               └── Дочерние виджеты(включая _ui)
    │               └── brief Оператор присваивания(запрещён для предотвращения копирования состояния)
    │               └── bool is(ReadyState state) [const noexcept]
    │               └── bool is(PlayState state) [const noexcept]
    │               └── bool is(ChangedState state) [const noexcept]
    │               └── bool is(ErrorType type) [const noexcept]
    │               └── note Использует прямое сравнение через геттер panel()
    │               └── так как перегрузка is(PanelState)
    │               └── bool is(PanelState panel) [const noexcept]
    │               └── note Использует прямое сравнение через геттер visibility()
    │               └── так как перегрузка is(VisibilityState)
    │               └── bool is(VisibilityState visibility) [const noexcept]
    │               └── const FCCondition& condition() [const noexcept]
    │               └── note При изменении состояния эмитируется сигнал conditionChanged()
    │               └── void set(const FCCondition &condition)
    │               └── void set(ReadyState state)
    │               └── void set(PlayState state)
    │               └── void set(ErrorType type)
    │               └── void set(PanelState panel)
    │               └── void set(VisibilityState visibility)
    │               └── param name Имя источника сообщения(например, "Plotter0", "Parser")
    │               └── void receiveMessage(const QString &name, const QString &message)
    │               └── param visibility Новое состояние видимости(Show/Hide)
    │               └── void console(VisibilityState visibility)
    │               └── Отображаются метаданные модели(размер, время печати)
    │               └── void onSelectModelsElement()
    │               └── эмитирует сигнал play()
    │               └── эмитирует сигнал pause()
    │               └── Визуально изменяет иконку кнопки(play.svg ↔ pause.svg)
    │               └── void onPlayPausePressed()
    │               └── void onChangePlotterPressed()
    │               └── param fileInfo Информация о файле модели(путь, размер, дата модификации)
    │               └── Превью изображения(если доступно)
    │               └── void onModelDataShow(const QFileInfo &fileInfo)
    │               └── brief Переключает активную панель интерфейса(главная/статистика/заставка)
    │               └── param panel Целевая панель(Main/Statistics/ScreenSaver)
    │               └── void onSwitchToPanel(PanelState panel)
    │               └── brief Обрабатывает прогресс парсинга модели(отображение в ProgressBar)
    │               └── param percent Процент завершения операции(0–100)
    │               └── void onParsingProcess(int percent)
    │               └── param name Имя плоттера для отображения в меню(например, "Plotter-USB0")
    │               └── void addPlotterName(const QString &name)
    │           └── 🔒 Защищённые методы:
    │               └── События касания(QTouchEvent)
    │               └── bool event(QEvent *event)
    │           └── 💾 Публичные члены-данные:
    │               └── FCDisplay& operator = (const FCDisplay&) = delete
    │               └── если текущее состояние готовности равно заданному
    │               └── если текущий режим воспроизведения равен заданному
    │               └── если флаг изменения равен заданному
    │               └── если текущий тип ошибки равен заданному
    │               └── если текущая панель равна заданной
    │               └── если состояние видимости равно заданному
    │           └── 🔊 Сигналы:
    │               └── void conditionChanged(const QString &name, const FCCondition &condition)
    │               └── void play(const QString &model)
    │               └── void pause()
    │               └── void stop()
    │           └── 🎯 Слоты:
    │               └── void receiveMessage(const QString &name, const QString &message)
    │               └── void console(VisibilityState visibility)
    │               └── void onSelectModelsElement()
    │               └── void onPlayPausePressed()
    │               └── void onChangePlotterPressed()
    │               └── void onModelDataShow(const QFileInfo &fileInfo)
    │               └── void onSwitchToPanel(PanelState panel)
    │               └── void onParsingProcess(int percent)
    │               └── void addPlotterName(const QString &name)
    ├── FCImages/
    ├── FCPlotter/
    │   ├── FCDevice/
    │   │   ├── FCI2CDevice/
    │   │   │   ├── FCHead/
    │   │   │   │   ├── FCNozzle/
    │   │   │   │   │   └── FCNozzle.h
    │   │   │   │   │       └── 📦 class FCNozzle
    │   │   │   │   │           └── 🏗️ Конструкторы:
    │   │   │   │   │               └── FCNozzle()
    │   │   │   │   │               └── FCNozzle(const FC2DPoint &position, float diameter, float rate)
    │   │   │   │   │           └── 📡 Публичные методы:
    │   │   │   │   │               └── : _position(0.0f, 0.0f)
    │   │   │   │   │               └──  _diameter(0.4f)
    │   │   │   │   │               └──  _flowRate(0.0f)
    │   │   │   │   │               └── param pos Позиция центра сопла в локальной системе координат головки(мм)
    │   │   │   │   │               └── param diam Диаметр отверстия сопла(мм)
    │   │   │   │   │               └── param rate Скорость потока материала(мм³/с)
    │   │   │   │   │               └── note При передаче недопустимого диаметра(≤ 0)
    │   │   │   │   │               └── : _position(position)
    │   │   │   │   │               └──  _diameter(diameter > 0.0f ? diameter : 0.1f)
    │   │   │   │   │               └── Защита от недопустимого диаметра _flowRate(rate >= 0.0f ? rate : 0.0f)
    │   │   │   │   │               └── brief Возвращает площадь поперечного сечения сопла(мм²)
    │   │   │   │   │               └── constexpr float crossSectionArea() [const noexcept]
    │   │   │   │   │           └── 💾 Публичные члены-данные:
    │   │   │   │   │               └── const float radius [const] = _diameter * 0.5f
    │   │   │   │   │               └── * radius * radius
    │   │   │   │   └── FCHead.h
    │   │   │   │       └── 📦 class FCHead
    │   │   │   │           └── 📌 Наследует: public FCI2CDevice
    │   │   │   │           └── 🏗️ Конструкторы:
    │   │   │   │               └── FCHead(FCI2CBus *bus, uint8_t address = 0, const QString &name = QString()
    │   │   │   │               └── FCHead()
    │   │   │   │           └── 🗑️ ~FCHead()
    │   │   │   │           └── 📡 Публичные методы:
    │   │   │   │               └── Количество рядов сопел в головке(всегда 1 для текущей конфигурации)
    │   │   │   │               └── Количество столбцов сопел в головке(всегда 3 для текущей конфигурации)
    │   │   │   │               └── Общее количество сопел в головке(произведение ROWS × COLS)
    │   │   │   │               └── param address Адрес устройства на шине I2C(по умолчанию 0 — автоматическое определение)
    │   │   │   │               └── param name Имя устройства для идентификации в системе(по умолчанию пустая строка)
    │   │   │   │               └── Сопло 0: позиция(5, 5)
    │   │   │   │               └── Сопло 1: позиция(15, 5)
    │   │   │   │               └── Сопло 2: позиция(5, 15)
    │   │   │   │               └── : FCI2CDevice(bus, address, name, parent)
    │   │   │   │           └── 💾 Публичные члены-данные:
    │   │   │   │               └── static constexpr int ROWS [const] = 1
    │   │   │   │               └── static constexpr int COLS [const] = 3
    │   │   │   │               └── static constexpr int NOZZLE_COUNT [const] = ROWS * COLS
    │   │   │   │               └── uint8_t address = 0, const QString &name = QString(), QObject *parent = nullptr)
        : FCI2CDevice(bus, address, name, parent)
    {
        // Инициализация списка сопел с фиксированной конфигурацией
        _nozzles.reserve(NOZZLE_COUNT)
    │   │   │   │           └── 🔑 Константы:
    │   │   │   │               └── int ROWS = 1
    │   │   │   │               └── int COLS = 3
    │   │   │   │               └── int NOZZLE_COUNT = ROWS * COLS
    │   │   │   ├── FCLM75A/
    │   │   │   │   ├── FCLM75AThermometer/
    │   │   │   │   │   └── FCLM75AThermometer.h
    │   │   │   │   │       └── 📦 class FCLM75AThermometer
    │   │   │   │   │           └── 📌 Наследует: public FCI2CDevice
    │   │   │   │   │           └── 🏗️ Конструкторы:
    │   │   │   │   │               └── FCLM75AThermometer(FCI2CBus *bus, uint8_t address = DEFAULT_I2C_ADDRESS, const QString &name = QString()
    │   │   │   │   │               └── FCLM75AThermometer()
    │   │   │   │   │           └── 🗑️ ~FCLM75AThermometer()
    │   │   │   │   │           └── 📡 Публичные методы:
    │   │   │   │   │               └── битного режима(мс)
    │   │   │   │   │               └── Разрешение по умолчанию(9 бит = 0.5 °C)
    │   │   │   │   │               └── Компенсация калибровочного смещения по умолчанию(°C)
    │   │   │   │   │               └── Порог изменения температуры для эмиссии сигнала temperatureChanged()
    │   │   │   │   │               └── Регистр температуры(только чтение, 16 бит)
    │   │   │   │   │               └── Регистр конфигурации(чтение/запись, 8 бит)
    │   │   │   │   │               └── Регистр гистерезиса(чтение/запись, 16 бит)
    │   │   │   │   │               └── Регистр порога срабатывания(чтение/запись, 16 бит)
    │   │   │   │   │               └── битный адрес датчика на шине(по умолчанию 0x48)
    │   │   │   │   │               └── param name Имя устройства для идентификации в системе(например, "AmbientSensor")
    │   │   │   │   │               └── Датчик остаётся в режиме отключения(для экономии энергии)
    │   │   │   │   │               └── до первого вызова temperatureC()
    │   │   │   │   │               └── Выполняется ГРУППОВАЯ установка состояния(один сигнал вместо трёх)
    │   │   │   │   │               └── : FCI2CDevice(bus, address, name, parent)
    │   │   │   │   │               └──  _compensation(DEFAULT_COMPENSATION)
    │   │   │   │   │               └──  _deltaThreshold(DEFAULT_DELTA_THRESHOLD)
    │   │   │   │   │               └──  _lastTemperature(std::numeric_limits<float>::quiet_NaN()
    │   │   │   │   │               └──  set({ReadyState::Ready, PlayState::Stop, ChangedState::NotChanged, ErrorType::NoError})
    │   │   │   │   │           └── 💾 Публичные члены-данные:
    │   │   │   │   │               └── Стандартный I²C адрес датчика при A0 = A1=A2=GND (7-битный адрес).
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x48
    │   │   │   │   │               └── static constexpr int MIN_CONVERSION_TIME_MS [const] = 27
    │   │   │   │   │               └── 9 бит = 0.5 °C).
    static constexpr int DEFAULT_RESOLUTION_BITS = 9
    │   │   │   │   │               └── static constexpr float DEFAULT_COMPENSATION [const] = 1.0f
    │   │   │   │   │               └── static constexpr float DEFAULT_DELTA_THRESHOLD [const] = 0.5f
    │   │   │   │   │               └── static constexpr uint8_t REG_TEMPERATURE [const] = 0x00
    │   │   │   │   │               └── static constexpr uint8_t REG_CONFIGURATION [const] = 0x01
    │   │   │   │   │               └── static constexpr uint8_t REG_THYST [const] = 0x02
    │   │   │   │   │               └── static constexpr uint8_t REG_TOS [const] = 0x03
    │   │   │   │   │               └── uint8_t address = DEFAULT_I2C_ADDRESS,
                                const QString &name = QString(), QObject *parent = nullptr)
        : FCI2CDevice(bus, address, name, parent),
          _compensation(DEFAULT_COMPENSATION),
          _deltaThreshold(DEFAULT_DELTA_THRESHOLD),
          _lastTemperature(std::numeric_limits<float>::quiet_NaN())
    {
        set({ReadyState::Ready, PlayState::Stop, ChangedState::NotChanged, ErrorType::NoError})
    │   │   │   │   │           └── 🔊 Сигналы:
    │   │   │   │   │               └── void temperatureChanged(float celsius)
    │   │   │   │   │           └── 🔑 Константы:
    │   │   │   │   │               └── uint8_t DEFAULT_I2C_ADDRESS = 0x48
    │   │   │   │   │               └── int MIN_CONVERSION_TIME_MS = 27
    │   │   │   │   │               └── int DEFAULT_RESOLUTION_BITS = 9
    │   │   │   │   │               └── float DEFAULT_COMPENSATION = 1.0f
    │   │   │   │   │               └── float DEFAULT_DELTA_THRESHOLD = 0.5f
    │   │   │   │   │               └── uint8_t REG_TEMPERATURE = 0x00
    │   │   │   │   │               └── uint8_t REG_CONFIGURATION = 0x01
    │   │   │   │   │               └── uint8_t REG_THYST = 0x02
    │   │   │   │   │               └── uint8_t REG_TOS = 0x03
    │   │   │   │   └── LM75A.h
    │   │   │   │       └── 📦 struct LM75A
    │   │   │   ├── FCPumpRamp/
    │   │   │   │   └── FCPumpRamp.h
    │   │   │   │       └── 📦 class FCPumpRamp
    │   │   │   │           └── 📌 Наследует: public FCI2CDevice
    │   │   │   │           └── 🏗️ Конструкторы:
    │   │   │   │               └── FCPumpRamp(FCI2CBus *bus, QObject *parent = nullptr) : FCI2CDevice(bus, 0x20, "PumpRamp", parent),
      ...
    │   │   │   │               └── FCPumpRamp()
    │   │   │   │           └── 🗑️ ~FCPumpRamp()
    │   │   │   │           └── 📡 Публичные методы:
    │   │   │   │               └── param bus Указатель на шину I²C(должна быть инициализирована заранее)
    │   │   │   │               └── Все насосы отключаются через вызов reset()
    │   │   │   │               └── : FCI2CDevice(bus, 0x20, "PumpRamp", parent)
    │   │   │   │               └── Создание и инициализация датчиков температуры(владеет классом через родительство)
    │   │   │   │               └── new FCLM75AThermometer(bus, 0x30, "t0", this)
    │   │   │   │               └── new FCLM75AThermometer(bus, 0x31, "t1", this)
    │   │   │   │               └── new FCLM75AThermometer(bus, 0x32, "t2", this)
    │   │   │   │               └── Отключение всех насосов при инициализации reset()
    │   │   │   │           └── 🔊 Сигналы:
    │   │   │   │               └── void pumpSwitched(uint8_t pumpNumber)
    │   │   │   │               └── void temperatureChanged(uint8_t index, qreal temperature)
    │   │   │   └── FCI2CDevice.h
    │   │   │       └── 📦 class FCI2CDevice
    │   │   │           └── 📌 Наследует: public FCDevice
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FCI2CDevice(FCI2CBus *bus, uint8_t address, const QString &name = QString()
    │   │   │               └── FCI2CDevice()
    │   │   │           └── 🗑️ ~FCI2CDevice()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── param bus Указатель на шину I²C(должна быть инициализирована заранее)
    │   │   │               └── битный адрес устройства на шине(0x00–0x7F)
    │   │   │               └── Выполняется валидация параметров(шину и адрес)
    │   │   │               └── : FCDevice(name, parent)
    │   │   │               └── 0x7F if(!_bus || !FCRange<uint8_t>(0x00, 0x7F)
    │   │   │               └──  set({ReadyState::Error, PlayState::Stop, ChangedState::Changed, ErrorType::ConnectionError})
    │   │   │           └── 🔒 Защищённые методы:
    │   │   │               └── uint8_t readByte()
    │   │   │               └──  if(!_bus)
    │   │   │               └──  set({ReadyState::Error, PlayState::Stop, ChangedState::Changed, ErrorType::ConnectionError})
    │   │   ├── FCSerialDevice/
    │   │   │   ├── FCMarlinController/
    │   │   │   │   ├── FCM115/
    │   │   │   │   │   └── FCM115.h
    │   │   │   │   │       └── 📦 class FCM115
    │   │   │   │   │           └── 🏗️ Конструкторы:
    │   │   │   │   │               └── FCM115()
    │   │   │   │   │               └── FCM115(const QString &response)
    │   │   │   │   │           └── 📡 Публичные методы:
    │   │   │   │   │               └── brief Конструктор по умолчанию(пустые данные, невалидный объект)
    │   │   │   │   │               └── note При ошибке парсинга объект остаётся в невалидном состоянии(isValid()
    │   │   │   │   │               └── bool parse(const QString &response)
    │   │   │   │   │               └── bool isValid() [const noexcept]
    │   │   │   │   │               └── bool isCompatible() [const noexcept]
    │   │   │   │   │               └── const QString& firmware() [const noexcept]
    │   │   │   │   │               └── const QString& version() [const noexcept]
    │   │   │   │   │               └── const QString& machine() [const noexcept]
    │   │   │   │   │               └── const FC3DArea& area() [const noexcept]
    │   │   │   │   │               └── const FCSpeed& speeds() [const noexcept]
    │   │   │   │   │               └── const QString& uuid() [const noexcept]
    │   │   │   │   │           └── 💾 Публичные члены-данные:
    │   │   │   │   │               └── static constexpr const char* EXPECTED_UUID [const] = "50ca114a-fdc8-41e0-9912-bfe7ec3d20bb"
    │   │   │   │   │               └── return true при успешном парсинге всех критических параметров
    │   │   │   │   │               └── return true если все обязательные поля заполнены
    │   │   │   │   │               └── return _isValid
    │   │   │   │   │               └── return true если UUID совпадает с ожидаемым
    │   │   │   │   │               └── return _uuid = = QString::fromUtf8(EXPECTED_UUID)
    │   │   │   │   │               └── return _firmware
    │   │   │   │   │               └── return _version
    │   │   │   │   │               └── return _machine
    │   │   │   │   │               └── return _area
    │   │   │   │   │               └── return _speeds
    │   │   │   │   │               └── return _uuid
    │   │   │   │   └── FCMarlinController.h
    │   │   │   │       └── 📦 class FCMarlinController
    │   │   │   │           └── 📌 Наследует: public FCSerialDevice
    │   │   │   │           └── 🏗️ Конструкторы:
    │   │   │   │               └── FCMarlinController(const QString &portName, QObject *parent = nullptr)
    │   │   │   │               └── FCMarlinController()
    │   │   │   │           └── 🗑️ ~FCMarlinController()
    │   │   │   │           └── 📡 Публичные методы:
    │   │   │   │               └── param portName Имя последовательного порта(например, "/dev/ttyAMA0" на Raspberry Pi)
    │   │   │   │               └── param bus Указатель на шину I2C для управления периферийными устройствами(может быть nullptr)
    │   │   │   │               └── Для установки соединения необходимо вызвать метод connect()
    │   │   │   │               └── записи текущим пользователем(права uucp/dialout)
    │   │   │   │               └── note Отправляет команду M105(запрос температуры)
    │   │   │   │               └── warning Блокирует вызывающий поток на время ожидания ответа(до 1 секунды)
    │   │   │   │               └── специфичная проверка через команду M105(запрос температуры)
    │   │   │   │               └── inline bool checkConnection()
    │   │   │   │               └── return sendCommand(("M105")
    │   │   │   │               └── обновлены вручную через внутренние механизмы(не экспонируются напрямую)
    │   │   │   │               └── inline const FCM115& firmwareData() [const noexcept]
    │   │   │   │               └── param colors Список цветов для последовательного отображения на ленте(RGB)
    │   │   │   │               └── param brightness Яркость ленты(0–255, где 255 = максимальная яркость)
    │   │   │   │               └── Для адресных лент(NeoPixel)
    │   │   │   │               └── bool setLed(const QList<QColor> &colors, uint8_t brightness = 255)
    │   │   │   │               └── аварийная остановка в Marlin(немедленный сброс буферов)
    │   │   │   │               └── inline void emergencyStop()
    │   │   │   │               └── отключить шаговые двигатели в Marlin
    inline void disableMotors()
    │   │   │   │               └── Примечание: после выполнения требуется повторное подключение
    inline void reboot()
    │   │   │   │               └── QString securityCode(int timeoutMs = DEFAULT_TIMEOUT_MS) [override]
    │   │   │   │           └── 🔒 Защищённые методы:
    │   │   │   │               └── param response Полная строка ответа от контроллера(включая "\n")
    │   │   │   │               └── note Переопределяет базовый parseResponse()
    │   │   │   │               └── информационные сообщения(игнорируются)
    │   │   │   │               └── bool parse(const QString &response)
    │   │   │   │               └── note Переопределяет базовый setupPortParameters(QSerialPort *port)
    │   │   │   │               └── Скорость: 115200 бод(стандарт для Marlin)
    │   │   │   │               └── Формат: 8N1(8 бит, без чётности, 1 стоп-бит)
    │   │   │   │               └── bool setupPortParameters(QSerialPort *port)
    │   │   │   │           └── 💾 Публичные члены-данные:
    │   │   │   │               └── в течение таймаута
    │   │   │   │               └── return _m115
    │   │   │   │               └── где 255 = максимальная яркость).
    /// @return true если команда управления лентой успешно отправлена и подтверждена
    │   │   │   │               └── uint8_t brightness = 255)
    │   │   │   │               └── int timeoutMs = DEFAULT_TIMEOUT_MS) override
    │   │   │   │           └── 🔊 Сигналы:
    │   │   │   │               └── void colorChanged(const QColor &color)
    │   │   │   └── FCSerialDevice.h
    │   │   │       └── 📦 class FCSerialDevice
    │   │   │           └── 📌 Наследует: public FCDevice
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FCSerialDevice(const QString &portName, QObject *parent = nullptr)
    │   │   │               └── FCSerialDevice()
    │   │   │           └── 🗑️ ~FCSerialDevice()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── param portName Имя последовательного порта(например, "/dev/ttyAMA0" на Raspberry Pi)
    │   │   │               └── Для установки соединения необходимо вызвать метод connect()
    │   │   │               └── отправка команд(ВСЕГДА с подтверждением)
    │   │   │               └── param command Байтовый массив команды(без символа новой строки)
    │   │   │               └── param timeoutMs Таймаут ожидания подтверждения в миллисекундах(по умолчанию 1000 мс)
    │   │   │               └── param retries Количество повторных попыток при таймауте(по умолчанию 3)
    │   │   │               └── При таймауте выполняется повторная отправка(до retries попыток)
    │   │   │               └── bool sendCommand(const QByteArray &command, int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES)
    │   │   │               └── param commands Список команд для отправки(каждая как QByteArray)
    │   │   │               └── param timeoutMs Таймаут для каждой команды(по умолчанию 1000 мс)
    │   │   │               └── param retries Количество повторных попыток для каждой команды(по умолчанию 3)
    │   │   │               └── bool sendCommands(const QByteArrayList &commands, int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES)
    │   │   │               └── bool connect()
    │   │   │               └── void disconnect()
    │   │   │           └── 🔒 Защищённые методы:
    │   │   │               └── Таймаут ожидания подтверждения команды(в миллисекундах)
    │   │   │               └── brief Низкоуровневая отправка команды с ожиданием(без повторных попыток)
    │   │   │               └── note Используется внутри sendCommand()
    │   │   │               └── bool sendCommandRaw(const QString &command, int timeoutMs)
    │   │   │               └── void flush()
    │   │   │               └── param portName Имя порта для проверки(например, "/dev/ttyAMA0" или "COM3")
    │   │   │               └── операция O(n)
    │   │   │               └── Рекомендуется вызывать перед connect()
    │   │   │               └── bool available()
    │   │   │               └── Скорость: 115200 бод(или другая, специфичная для устройства)
    │   │   │               └── Формат: 8N1(8 бит данных, без чётности, 1 стоп-бит)
    │   │   │               └── Управление потоком: NoFlowControl(программное через таймауты)
    │   │   │               └── но ДО открытия порта методом open()
    │   │   │               └── virtual bool setupPortParameters(QSerialPort *port) [virtual]
    │   │   │               └── новой команды или при явном вызове flush()
    │   │   │               └── что команда уже отправлена и подтверждение получено(например, после
    ///          успешного выполнения sendCommand()
    │   │   │               └── что буфер не модифицируется одновременно(гарантируется архитектурой класса)
    │   │   │               └── const QString answer()
    │   │   │               └── const QString answer()
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── return true если команда отправлена И получено подтверждение
    │   │   │               └── int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES)
    │   │   │               └── return true если ВСЕ команды получили подтверждение
    │   │   │               └── int timeoutMs = DEFAULT_TIMEOUT_MS, int retries = MAX_RETRIES)
    │   │   │               └── return true при успешном открытии порта и подтверждении связи
    │   │   │           └── 🔊 Сигналы:
    │   │   │               └── void connected()
    │   │   │               └── void disconnected()
    │   │   │               └── void error(const FCSerialDevice &device, const QString &details)
    │   │   │           └── 🔑 Константы:
    │   │   │               └── int MAX_RETRIES = 3
    │   │   │               └── int DEFAULT_TIMEOUT_MS = 1000
    │   │   └── FCDevice.h
    │   │       └── 📦 class FCDevice
    │   │           └── 📌 Наследует: public FCConditionObject
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCDevice(const QString &name = QString()
    │   │               └── FCDevice()
    │   │               └── FCDevice(const FCDevice&)
    │   │               └── FCDevice(const FCDevice&)(const FCDevice& other)
    │   │           └── 🗑️ ~FCDevice()
    │   │           └── 📡 Публичные методы:
    │   │               └── : FCConditionObject(name, parent)
    │   └── FCPlotter.h
    │       └── 📦 class FCPlotter
    │           └── 📌 Наследует: public FCDevice
    │           └── 🏗️ Конструкторы:
    │               └── FCPlotter(const QString &portName, FCI2CBus *bus, QObject *parent = nullptr)
    │               └── FCPlotter()
    │           └── 🗑️ ~FCPlotter()
    │           └── 📡 Публичные методы:
    │               └── < Длительность длинного теста(2 минуты)
    │               └── param portName Имя порта для подключения к контроллеру(например, "/dev/ttyUSB0")
    │               └── Объект перемещается в рабочий поток через moveToThread()
    │               └── Инициализируются компоненты управления(контроллер, насосная рампа)
    │               └── Ожидание завершения всех операций(макс. 5 секунд)
    │               └── Освобождение всех ресурсов(контроллер, насосная рампа, буферы)
    │               └── warning Блокирует вызывающий поток на время ожидания завершения(до 5 секунд)
    │               └── bool startThread()
    │               └── Ожидание завершения всех операций(макс. 5 секунд)
    │               └── warning Блокирует вызывающий поток на время ожидания завершения(до 5 секунд)
    │               └── bool stopThread()
    │               └── Управление операциями нанесения(неблокирующие)
    │               └── param container Контейнер с данными векторной модели(слои, фигуры, элементы)
    │               └── Прогресс выполнения отслеживается через
     *       сигналы состояния(conditionChanged)
    │               └── и диагностические сообщения(sigMessage)
    │               └── Q_INVOKABLE void start(const FCSVGImageContainer &container)
    │               └── Q_INVOKABLE void stop()
    │               └── Q_INVOKABLE void pause()
    │               └── Q_INVOKABLE void reset()
    │               └── Сервисные операции(неблокирующие)
    │               └── Q_INVOKABLE void clear()
    │               └── brief Запускает короткий самотест(быстрая диагностика критических компонентов)
    │               └── Результат возвращается через сигнал sigTestResult()
    │               └── Q_INVOKABLE void shortTest()
    │               └── brief Запускает полный самотест(расширенная диагностика всех систем)
    │               └── Результат возвращается через сигнал sigTestResult()
    │               └── Q_INVOKABLE void longTest()
    │               └── QString serialNumber() [const noexcept]
    │               └── bool isThreadRunning() [const noexcept]
    │               └── param timeoutMs Таймаут ожидания ответа(мс)
    │               └── QString securityCode(int timeoutMs) [override]
    │           └── 💾 Публичные члены-данные:
    │               └── static constexpr int PLOTTER_THREAD_STOP_TIMEOUT_MS [const] = 5000
    │               └── < Таймаут ожидания завершения потока
    static constexpr int PLOTTER_COMMAND_PROCESS_INTERVAL_MS [const] = 10
    │               └── < Интервал обработки команд в рабочем цикле
    static constexpr int PLOTTER_SHORT_TEST_DURATION_MS [const] = 3000
    │               └── < Длительность короткого теста
    static constexpr int PLOTTER_LONG_TEST_DURATION_MS [const] = 120000
    │               └── static constexpr int PLOTTER_CLEAR_DURATION_MS [const] = 2000
    │               └── < Длительность очистки головки
    static constexpr int PLOTTER_PROGRESS_UPDATE_INTERVAL_MS [const] = 500
    │               └── return true при успешном запуске потока
    │               └── return true при корректном завершении
    │               └── return _serialNumber
    │               └── return true если поток активен и работает
    │               └── const noexcept [const]
    │           └── 🔊 Сигналы:
    │               └── void message(const QString &name, const QString &message)
    │               └── void error(const QString &name, const QString &message)
    │               └── void progress(int percent, int layer = -1)
    │               └── void test(const QString &testName, bool success, const QString &details)
    │               └── void run()
    │           └── 🎯 Слоты:
    │               └── void run()
    │           └── 🔑 Константы:
    │               └── int PLOTTER_THREAD_STOP_TIMEOUT_MS = 5000
    │               └── int PLOTTER_COMMAND_PROCESS_INTERVAL_MS = 10
    │               └── int PLOTTER_SHORT_TEST_DURATION_MS = 3000
    │               └── int PLOTTER_LONG_TEST_DURATION_MS = 120000
    │               └── int PLOTTER_CLEAR_DURATION_MS = 2000
    │               └── int PLOTTER_PROGRESS_UPDATE_INTERVAL_MS = 500
    ├── FCSSI/
    │   └── FCI2CBus/
    │       └── FCI2CBus.h
    │           └── 📦 class FCI2CBus
    │               └── 📌 Наследует: public FCConditionObject
    │               └── 🏗️ Конструкторы:
    │                   └── FCI2CBus(const QString &path, QObject *parent = nullptr) : FCConditionObject(path, parent),
          _file(p...
    │                   └── FCI2CBus()
    │                   └── FCI2CBus(const FCI2CBus&)
    │                   └── FCI2CBus(const FCI2CBus&)(const FCI2CBus& other)
    │               └── 🗑️ ~FCI2CBus()
    │               └── 📡 Публичные методы:
    │                   └── param path Путь к устройству шины в файловой системе(например, "/dev/i2c-1")
    │                   └── Вызывается метод open()
    │                   └── : FCConditionObject(path, parent)
    │                   └──  _file(path)
    │                   └──  if(open()
    │                   └──  set({ReadyState::Ready, PlayState::Stop, ChangedState::NotChanged, ErrorType::NoError})
    ├── FCSVGImage/
    │   ├── FCSVGImageCoder/
    │   │   └── FCSVGImageCoder.h
    │   │       └── 📦 class FCSVGImageCoder
    │   │           └── 📌 Наследует: public FCDevice
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCSVGImageCoder(const QString &name, QObject *parent = nullptr)
    │   │               └── FCSVGImageCoder()
    │   │           └── 🗑️ ~FCSVGImageCoder()
    │   │           └── 📡 Публичные методы:
    │   │               └── Объект перемещается в рабочий поток через moveToThread()
    │   │               └── Инициализируется парсер SVG(FCSVGImageParser)
    │   │               └── Ожидание завершения всех операций(макс. 5 секунд)
    │   │               └── warning Блокирует вызывающий поток на время ожидания(до 5 секунд)
    │   │               └── bool startThread()
    │   │               └── bool stopThread()
    │   │               └── Q_INVOKABLE void decode(const QString &svgFilePath)
    │   │               └── Q_INVOKABLE void encode(const FCSVGImageContainer &container, const QString &outputFilePath)
    │   │               └── Q_INVOKABLE void cancel()
    │   │               └── bool isThreadRunning() [const noexcept]
    │   │               └── param timeoutMs Таймаут ожидания ответа(мс)
    │   │               └── QString securityCode(int timeoutMs) [override]
    │   │           └── 💾 Публичные члены-данные:
    │   │               └── static constexpr int CODER_THREAD_STOP_TIMEOUT_MS [const] = 5000
    │   │               └── < Таймаут ожидания завершения потока
    static constexpr int CODER_COMMAND_PROCESS_INTERVAL_MS [const] = 10
    │   │               └── < Интервал обработки команд
    static constexpr int CODER_PROGRESS_UPDATE_INTERVAL_MS [const] = 100
    │   │               └── return true при успешном запуске потока
    │   │               └── return true при корректном завершении
    │   │               └── return true если поток активен и работает
    │   │               └── const noexcept [const]
    │   │           └── 🔊 Сигналы:
    │   │               └── void message(const QString &name, const QString &message)
    │   │               └── void sigDecodeFinished(const FCSVGImageContainer &container, bool success, const QString &details)
    │   │               └── void sigEncodeFinished(const QString &outputPath, bool success, const QString &details)
    │   │               └── void progress(int percent, const QString &stage)
    │   │               └── void run()
    │   │           └── 🎯 Слоты:
    │   │               └── void run()
    │   │           └── 🔑 Константы:
    │   │               └── int CODER_THREAD_STOP_TIMEOUT_MS = 5000
    │   │               └── int CODER_COMMAND_PROCESS_INTERVAL_MS = 10
    │   │               └── int CODER_PROGRESS_UPDATE_INTERVAL_MS = 100
    │   ├── FCSVGImageContainer/
    │   │   └── FCSVGImageContainer.h
    │   │       └── 📦 class FCSVGImageContainer
    │   │           └── 📌 Наследует: public FCConditionObject
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCSVGImageContainer(QObject *parent = nullptr)
    │   │               └── FCSVGImageContainer(const QString &name, QObject *parent = nullptr)
    │   │               └── FCSVGImageContainer(const FCSVGImageContainer &other, QObject *parent = nullptr)
    │   │               └── FCSVGImageContainer()
    │   │               └── FCSVGImageContainer(const FCSVGImageContainer&)(const FCSVGImageContainer& other)
    │   │           └── 🗑️ ~FCSVGImageContainer()
    │   │           └── 📡 Публичные методы:
    │   │               └── brief Размер изображения в мм(из viewBox или расчётный)
    │   │               └── brief Доступная рабочая область плоттера(из конфигурации)
    │   │               └── brief Оригинальный viewBox из SVG(для масштабирования координат)
    │   │               └── brief Количество слоёв(inkscape:layer)
    │   │               └── brief Преобладающий цвет(для multi-color печати)
    │   │               └── brief Время завершения парсинга(для кэширования)
    │   │               └── brief Версия парсера(для совместимости форматов)
    │   │               └── brief Конструктор по умолчанию(нулевые значения)
    │   │               └──  Metadata()
    │   │               └── : figureCount(0)
    │   │           └── 💾 Публичные члены-данные:
    │   │               └── QString sourceFile
    │   │               └── QFileInfo fileInfo
    │   │               └── права доступа
        FC2DSize imageSize
    │   │               └── FC2DSize workingArea
    │   │               └── FC2DSize viewBox
    │   │               └── quint32 figureCount
    │   │               └── brief Общее количество распарсенных фигур
        quint32 layerCount
    │   │               └── quint64 totalPoints
    │   │               └── brief Суммарное количество точек во всех контурах
        QColor dominantColor
    │   │               └── qreal inkCoverage
    │   │               └── для расчёта времени
        QDateTime parsedAt
    │   │               └── QString parserVersion
    │   │           └── 🔊 Сигналы:
    │   │               └── void containerReady()
    │   │               └── void containerError(ErrorType error, const QString &details)
    │   │               └── void writeProgress(int percent)
    │   │               └── void layerAdded(quint32 layerIndex)
    │   │               └── void figureAdded(quint32 layerIndex, quint32 figureIndex)
    │   │       └── 📦 struct Metadata
    │   │           └── 🏗️ Конструкторы:
    │   │               └── Metadata() : figureCount(0)
            , layerCount(0)
       ...
    │   │       └── 📦 struct BinaryFigure
    │   │           └── 🏗️ Конструкторы:
    │   │               └── BinaryFigure() : id(0)
            , layerIndex(0)
            , th...
    │   │       └── 📦 struct BinaryLayer
    │   │           └── 🏗️ Конструкторы:
    │   │               └── BinaryLayer() : index(0)
            , zPosition(0.0)
            ...
    │   └── FCSVGImageParser/
    │       ├── FCSVGFigure/
    │       │   └── FCSVGFigure.h
    │       │       └── 📦 class FCSVGFigure
    │       │           └── 🏗️ Конструкторы:
    │       │               └── FCSVGFigure(const QString &name = QString()
    │       │               └── FCSVGFigure(const FCSVGFigure &)
    │       │               └── FCSVGFigure(FCSVGFigure &&)
    │       │               └── FCSVGFigure()
    │       │               └── FCSVGFigure(const FCSVGFigure&)(const FCSVGFigure& other)
    │       │               └── FCSVGFigure(FCSVGFigure&&)(FCSVGFigure&& other)
    │       │           └── 🗑️ ~FCSVGFigure()
    │       │           └── 📡 Публичные методы:
    │       │               └── param name Имя фигуры(например, из атрибута id в SVG)
    │       │               └── param pen Перо для отрисовки контура(цвет, толщина, стиль)
    │       │               └── param brush Кисть для заполнения области(цвет, градиент, узор)
    │       │               └── param path Геометрический путь фигуры(последовательность точек)
    │       │               └── noexcept
        : _name(name)
    │       │               └──  _pen(pen)
    │       │               └──  _brush(brush)
    │       │               └──  _path(path)
    │       │               └── brief Конструктор копирования(генерируется компилятором)
    │       │               └── brief Конструктор перемещения(генерируется компилятором)
    │       │               └── brief Деструктор(генерируется компилятором)
    │       │               └── return Константная ссылка на имя(из атрибута id в SVG)
    │       │               └── constexpr bool isEmpty() [const noexcept]
    │       │               └── constexpr int count() [const noexcept]
    │       │               └── return QRectF с координатами границ(x, y, width, height)
    │       │               └── QRectF boundingRect() [const noexcept]
    │       │               └── param index Индекс элемента пути(0-based)
    │       │               └── возвращается точка(0, 0)
    │       │               └── FC2DPoint pointAt(int index) [const noexcept]
    │       │               └──  if(index < 0 || index >= _path.elementCount()
    │       │               └── return FC2DPoint(0.0, 0.0)
    │       │           └── 💾 Публичные члены-данные:
    │       │               └── QPen pen = QPen(), QBrush brush = QBrush(), QPainterPath path = QPainterPath()) noexcept
        : _name(name),
          _pen(pen),
          _brush(brush),
          _path(path)
    {}

    /**
     * @brief Конструктор копирования (генерируется компилятором).
     * @note Тривиальная копия всех полей.
     */
    FCSVGFigure(const FCSVGFigure &) = default
    │       │               └── return _name
    │       │               └── return _pen
    │       │               └── return _brush
    │       │               └── return _path
    │       │               └── return true если путь не содержит элементов
    │       ├── FCSVGLayer/
    │       │   └── FCSVGLayer.h
    │       │       └── 📦 class FCSVGLayer
    │       │           └── 📌 Наследует: public FCConditionObject
    │       │           └── 🏗️ Конструкторы:
    │       │               └── FCSVGLayer(const QString &name = QString()
    │       │               └── FCSVGLayer()
    │       │               └── FCSVGLayer(const FCSVGLayer &)
    │       │               └── FCSVGLayer(FCSVGLayer &&)
    │       │               └── FCSVGLayer(const FCSVGLayer&)(const FCSVGLayer& other)
    │       │               └── FCSVGLayer(FCSVGLayer&&)(FCSVGLayer&& other)
    │       │           └── 🗑️ ~FCSVGLayer()
    │       │           └── 📡 Публичные методы:
    │       │               └── param name Имя слоя(из атрибута label в Inkscape)
    │       │               └── param thickness Толщина слоя в мм(для Z-координаты плоттера)
    │       │               └── : FCConditionObject(name, parent)
    │       │               └──  _thickness(thickness)
    │       │               └──  _zPosition(0.0)
    │       │               └── brief Деструктор(генерируется компилятором)
    │       │               └── Запрет копирования и перемещения(QObject не поддерживает)
    │       │               └── return Толщина в мм(для Z-координаты плоттера)
    │       │               └── constexpr qreal thickness() [const noexcept]
    │       │               └── return Координата Z в мм(для G-кода)
    │       │               └── constexpr qreal zPosition() [const noexcept]
    │       │               └── brief Возвращает список всех фигур в слое(для модификации)
    │       │               └── int count() [const noexcept]
    │       │               └── param index Индекс фигуры(0-based)
    │       │           └── 💾 Публичные члены-данные:
    │       │               └── qreal thickness = 1.0, QObject *parent = nullptr)
        : FCConditionObject(name, parent),
          _thickness(thickness),
          _zPosition(0.0)
    {}

    /**
     * @brief Деструктор (генерируется компилятором).
     * @note QObject автоматически удалит дочерние объекты.
     */
    ~FCSVGLayer() = default
    │       │               └── return _thickness
    │       │               └── return _zPosition
    │       │               └── return _figures
    │       │               └── return _figures
    │       └── FCSVGImageParser.h
    │           └── 📦 class FCSVGImageParser
    │               └── 📌 Наследует: public FCConditionObject
    │               └── 🏗️ Конструкторы:
    │                   └── FCSVGImageParser(QObject *parent = nullptr)
    │                   └── FCSVGImageParser(const ParserSettings &settings, QObject *parent = nullptr)
    │                   └── FCSVGImageParser()
    │               └── 🗑️ ~FCSVGImageParser()
    │               └── 📡 Публичные методы:
    │                   └── brief Допуск упрощения путей(мм)
    │                   └── brief Применять алгоритм упрощения(Ramer-Douglas-Peucker)
    │                   └── brief Минимальная длина сегмента(мм)
    │                   └── brief Преобразовывать текстовые элементы(<text>)
    │                   └── brief Игнорировать группировку(<g>)
    │                   └── brief Цвет фона для обработки прозрачных областей(альфа-композитинг)
    │                   └──  ParserSettings()
    │                   └── : tolerance(0.01)
    │               └── 💾 Публичные члены-данные:
    │                   └── qreal tolerance
    │                   └── bool simplifyPaths
    │                   └── bool mergeAdjacent
    │                   └── bool removeDuplicates
    │                   └── qreal minSegmentLength
    │                   └── bool extractText
    │                   └── bool flattenGroups
    │                   └── QColor backgroundColor
    │                   └── bool ignoreHiddenLayers
    │               └── 🔊 Сигналы:
    │                   └── void parsingStarted(const QString &filePath)
    │                   └── void parsingProgress(int percent, const QString &currentStage)
    │                   └── void parsingFinished(const FCSVGImageParser::ParseResult &result)
    │                   └── void parsingError(const QString &error, int line = -1)
    │                   └── void containerReady(FCSVGImageContainer *container)
    │                   └── void onParsingFinished()
    │                   └── void onParsingProgress()
    │               └── 🎯 Слоты:
    │                   └── void onParsingFinished()
    │                   └── void onParsingProgress()
    │           └── 📦 struct ParserSettings
    │               └── 🏗️ Конструкторы:
    │                   └── ParserSettings() : tolerance(0.01)           ///< 10 микрон — точност...
    │           └── 📦 struct ParseResult
    │               └── 🏗️ Конструкторы:
    │                   └── ParseResult() : success(false)
            , figuresParsed(0)
    ...
    ├── FCTypes/
    │   ├── FC2DArea/
    │   │   ├── FC2DCell/
    │   │   │   └── FC2DCell.h
    │   │   │       └── 📦 class FC2DCell
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FC2DCell()
    │   │   │               └── FC2DCell(const FC2DPoint& position, const FC2DSize& size)
    │   │   │               └── FC2DCell(const FC2DCell&)
    │   │   │               └── FC2DCell()
    │   │   │               └── FC2DCell(const FC2DCell&)(const FC2DCell& other)
    │   │   │           └── 🗑️ ~FC2DCell()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── используемый для хранения координат и размеров(по умолчанию — float)
    │   │   │               └── Создаёт ячейку с позицией(0, 0)
    │   │   │               └── param size Размеры ячейки(ширина и высота)
    │   │   │               └── : _position(position)
    │   │   │               └── brief Конструктор копирования(тривиальный)
    │   │   │               └── brief Оператор присваивания(тривиальный)
    │   │   │               └── brief Деструктор(тривиальный)
    │   │   │               └── constexpr const FC2DPoint& position() [const noexcept]
    │   │   │               └── constexpr const FC2DSize& size() [const noexcept]
    │   │   │               └── constexpr _type x() [noexcept]
    │   │   │               └── constexpr _type y() [noexcept]
    │   │   │               └── constexpr _type width() [const noexcept]
    │   │   │               └── constexpr _type height() [const noexcept]
    │   │   │               └── constexpr void set(const FC2DPoint &p) [noexcept]
    │   │   │               └── constexpr void set(const FC2DSize &size) [noexcept]
    │   │   │               └── brief Перемещает ячейку на заданный вектор(dx, dy)
    │   │   │               └── constexpr void move(_type dx, _type dy) [noexcept]
    │   │   │               └── brief Перемещает ячейку на заданный вектор(объект FC2DPoint)
    │   │   │               └── constexpr void move(const FC2DPoint &delta) [noexcept]
    │   │   │               └── constexpr FC2DPoint center() [const noexcept]
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── constexpr FC2DCell& operator [const] = (const FC2DCell&) noexcept = default
    │   │   │               └── return _position
    │   │   │               └── return _size
    │   │   ├── FC2DPoint/
    │   │   │   └── FC2DPoint.h
    │   │   │       └── 📦 class FC2DPoint
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FC2DPoint()
    │   │   │               └── FC2DPoint(_type x, _type y)
    │   │   │               └── FC2DPoint(FC3DPoint& point)
    │   │   │               └── FC2DPoint(const FC2DPoint&)
    │   │   │               └── FC2DPoint()
    │   │   │               └── FC2DPoint(const FC2DPoint&)(const FC2DPoint& other)
    │   │   │           └── 🗑️ ~FC2DPoint()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── используемый для хранения координат(по умолчанию — float)
    │   │   │               └── Создаёт точку с координатами(0, 0)
    │   │   │               └── : _x(x)
    │   │   │               └── brief Конструктор копирования(тривиальный)
    │   │   │               └── brief Оператор присваивания копированием(тривиальный)
    │   │   │               └── brief Деструктор(тривиальный)
    │   │   │               └── constexpr _type x() [const noexcept]
    │   │   │               └── constexpr _type y() [const noexcept]
    │   │   │               └── constexpr void setX(_type x) [noexcept]
    │   │   │               └── constexpr void setY(_type y) [noexcept]
    │   │   │               └── constexpr void setXY(_type x, _type y) [noexcept]
    │   │   │               └── param z Значение координаты по оси Z(по умолчанию 0)
    │   │   │               └── constexpr FC3DPoint to3D(_type z = 0) [const noexcept]
    │   │   │               └── constexpr FC2DSize to2DSize() [const noexcept]
    │   │   │               └── constexpr FC2DPoint from2DSize(FC2DSize& size) [noexcept]
    │   │   │               └── _type distanceTo(const FC2DPoint& other) [const noexcept]
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── constexpr FC2DPoint& operator [const] = (const FC2DPoint&) noexcept = default
    │   │   │               └── return _x
    │   │   │               └── return _y
    │   │   │               └── _type z = 0) const noexcept
    │   │   │               └── где width = x, height = y.
    [[nodiscard]] constexpr FC2DSize to2DSize() const noexcept
    │   │   │               └── const _type dx [const] = _x - other._x
    │   │   │               └── const _type dy [const] = _y - other._y
    │   │   ├── FC2DSize/
    │   │   │   └── FC2DSize.h
    │   │   │       └── 📦 class FC2DSize
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FC2DSize()
    │   │   │               └── FC2DSize(_type w, _type h)
    │   │   │               └── FC2DSize(const FC2DSize&)
    │   │   │               └── FC2DSize()
    │   │   │               └── FC2DSize(const FC2DSize&)(const FC2DSize& other)
    │   │   │           └── 🗑️ ~FC2DSize()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── используемый для хранения размеров(по умолчанию — float)
    │   │   │               └── Создаёт размер с нулевыми значениями(0, 0)
    │   │   │               └── param w Ширина(значение по оси X)
    │   │   │               └── param h Высота(значение по оси Y)
    │   │   │               └── : _width(w)
    │   │   │               └──  _height(h)
    │   │   │               └── brief Конструктор копирования(тривиальный)
    │   │   │               └── brief Оператор присваивания копированием(тривиальный)
    │   │   │               └── brief Деструктор(тривиальный)
    │   │   │               └── constexpr _type width() [const noexcept]
    │   │   │               └── constexpr _type height() [const noexcept]
    │   │   │               └── constexpr void setWidth(_type w) [noexcept]
    │   │   │               └── constexpr void setHeight(_type h) [noexcept]
    │   │   │               └── constexpr void setWidthHeight(_type w, _type h) [noexcept]
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── constexpr FC2DSize& operator [const] = (const FC2DSize&) noexcept = default
    │   │   │               └── return _width
    │   │   │               └── return _height
    │   │   │               └──  _width = w
    │   │   │               └──  _height = h
    │   │   └── FC2DArea.h
    │   │       └── 📦 class FC2DArea
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FC2DArea()
    │   │               └── FC2DArea(const FC2DPoint& startPoint, const FC2DSize& areaSize = {1, 1}, const FC2DPoint& cellCounts = {1, 1})
    │   │               └── FC2DArea(const FC2DArea&)
    │   │               └── FC2DArea()
    │   │               └── FC2DArea(const FC2DArea&)(const FC2DArea& other)
    │   │           └── 🗑️ ~FC2DArea()
    │   │           └── 📡 Публичные методы:
    │   │               └── 1 с началом в(0, 0)
    │   │               └── param startPoint Начальная точка области(левый верхний угол)
    │   │               └── param areaSize   Размер всей области(по умолчанию 1×1)
    │   │               └── param cellCounts Количество ячеек по осям(Nx, Ny)
    │   ├── FC3DArea/
    │   │   ├── FC3DCell/
    │   │   │   └── FC3DCell.h
    │   │   │       └── 📦 class FC3DCell
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FC3DCell()
    │   │   │               └── FC3DCell(const FC3DPoint& position, const FC3DSize& size)
    │   │   │               └── FC3DCell(const FC3DCell&)
    │   │   │               └── FC3DCell()
    │   │   │               └── FC3DCell(const FC3DCell&)(const FC3DCell& other)
    │   │   │           └── 🗑️ ~FC3DCell()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── используемый для хранения координат и размеров(алиас qreal для совместимости с Qt)
    │   │   │               └── Создаёт ячейку с позицией(0, 0, 0)
    │   │   │               └── и нулевым размером(0, 0, 0)
    │   │   │               └── param position Позиция якорной точки ячейки(левый-верхний-ближний угол)
    │   │   │               └── param size Размеры ячейки(ширина, высота, глубина)
    │   │   │               └── : _position(position)
    │   │   │               └── brief Конструктор копирования(тривиальный)
    │   │   │               └── brief Оператор присваивания копированием(тривиальный)
    │   │   │               └── brief Деструктор(тривиальный)
    │   │   │               └── constexpr const FC3DPoint& position() [const noexcept]
    │   │   │               └── constexpr const FC3DSize& size() [const noexcept]
    │   │   │               └── constexpr _type x() [const noexcept]
    │   │   │               └── constexpr _type y() [const noexcept]
    │   │   │               └── constexpr _type z() [const noexcept]
    │   │   │               └── brief Возвращает ширину ячейки(размер по оси X)
    │   │   │               └── constexpr _type width() [const noexcept]
    │   │   │               └── brief Возвращает высоту ячейки(размер по оси Y)
    │   │   │               └── constexpr _type height() [const noexcept]
    │   │   │               └── brief Возвращает глубину ячейки(размер по оси Z)
    │   │   │               └── constexpr _type depth() [const noexcept]
    │   │   │               └── constexpr void setPosition(const FC3DPoint& position) [noexcept]
    │   │   │               └── constexpr void setSize(const FC3DSize& size) [noexcept]
    │   │   │               └── brief Перемещает ячейку на заданный вектор(dx, dy, dz)
    │   │   │               └── constexpr void move(_type dx, _type dy, _type dz) [noexcept]
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── constexpr FC3DCell& operator [const] = (const FC3DCell&) noexcept = default
    │   │   │               └── return _position
    │   │   │               └── return _size
    │   │   │               └──  _position = _position.translated(dx, dy, dz)
    │   │   ├── FC3DPoint/
    │   │   │   └── FC3DPoint.h
    │   │   │       └── 📦 class FC3DPoint
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FC3DPoint()
    │   │   │               └── FC3DPoint(_type x, _type y, _type z)
    │   │   │               └── FC3DPoint(FC2DPoint& point, _type z = 0)
    │   │   │               └── FC3DPoint(const FC3DPoint&)
    │   │   │               └── FC3DPoint()
    │   │   │               └── FC3DPoint(const FC3DPoint&)(const FC3DPoint& other)
    │   │   │           └── 🗑️ ~FC3DPoint()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── используемый для хранения координат(по умолчанию — float)
    │   │   │               └── Создаёт точку с координатами(0, 0, 0)
    │   │   │               └── : _x(x)
    │   │   │               └── param z Значение координаты по оси Z(по умолчанию 0)
    │   │   │               └── : _x(point.x()
    │   │   │               └──  _y(point.y()
    │   │   │               └──  _z(z)
    │   │   │               └── brief Конструктор копирования(тривиальный)
    │   │   │               └── brief Оператор присваивания копированием(тривиальный)
    │   │   │               └── brief Деструктор(тривиальный)
    │   │   │               └── constexpr _type x() [const noexcept]
    │   │   │               └── constexpr _type y() [const noexcept]
    │   │   │               └── constexpr _type z() [const noexcept]
    │   │   │               └── constexpr void setX(_type x) [noexcept]
    │   │   │               └── constexpr void setY(_type y) [noexcept]
    │   │   │               └── constexpr void setZ(_type z) [noexcept]
    │   │   │               └── constexpr void setXYZ(_type x, _type y, _type z) [noexcept]
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── _type z = 0) noexcept
        : _x(point.x()),
          _y(point.y()),
          _z(z)
    {}

    /// @brief Конструктор копирования (тривиальный).
    constexpr FC3DPoint(const FC3DPoint&) noexcept = default
    │   │   │               └── constexpr FC3DPoint& operator [const] = (const FC3DPoint&) noexcept = default
    │   │   │               └── return _x
    │   │   │               └── return _y
    │   │   │               └── return _z
    │   │   │               └──  _x = x
    │   │   │               └──  _y = y
    │   │   │               └──  _z = z
    │   │   ├── FC3DSize/
    │   │   │   └── FC3DSize.h
    │   │   │       └── 📦 class FC3DSize
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FC3DSize()
    │   │   │               └── FC3DSize(_type w, _type h, _type d)
    │   │   │               └── FC3DSize(const FC3DSize&)
    │   │   │               └── FC3DSize()
    │   │   │               └── FC3DSize(const FC3DSize&)(const FC3DSize& other)
    │   │   │           └── 🗑️ ~FC3DSize()
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── используемый для хранения размеров(по умолчанию — float)
    │   │   │               └── Создаёт размер с нулевыми значениями(0, 0, 0)
    │   │   │               └── param w Ширина(значение по оси X)
    │   │   │               └── param h Высота(значение по оси Y)
    │   │   │               └── param d Глубина(значение по оси Z)
    │   │   │               └── : _w(w)
    │   │   │               └── brief Конструктор копирования(тривиальный)
    │   │   │               └── brief Оператор присваивания копированием(тривиальный)
    │   │   │               └── brief Деструктор(тривиальный)
    │   │   │               └── return Значение ширины(компонента по оси X)
    │   │   │               └── constexpr _type width() [const noexcept]
    │   │   │               └── return Значение высоты(компонента по оси Y)
    │   │   │               └── constexpr _type height() [const noexcept]
    │   │   │               └── return Значение глубины(компонента по оси Z)
    │   │   │               └── constexpr _type depth() [const noexcept]
    │   │   │               └── constexpr void setWidth(_type w) [noexcept]
    │   │   │               └── constexpr void setHeight(_type h) [noexcept]
    │   │   │               └── constexpr void setDepth(_type d) [noexcept]
    │   │   │               └── constexpr void setWidthHeightDepth(_type w, _type h, _type d) [noexcept]
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── constexpr FC3DSize& operator [const] = (const FC3DSize&) noexcept = default
    │   │   │               └── return _w
    │   │   │               └── return _h
    │   │   │               └── return _d
    │   │   │               └──  _w = w
    │   │   │               └──  _h = h
    │   │   │               └──  _d = d
    │   │   └── FC3DArea.h
    │   │       └── 📦 class FC3DArea
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FC3DArea()
    │   │               └── FC3DArea(const FC3DPoint& startPoint, const FC3DSize& areaSize = FC3DSize{1, 1, 1}, const FC3DPoint& cellCounts = FC3DPoint{1, 1, 1})
    │   │               └── FC3DArea(qreal width, qreal depth, qreal height, qreal minZ, qreal maxZ, const FC3DPoint& cellCounts = FC3DPoint{1, 1, 1})
    │   │               └── FC3DArea(const FC3DArea&)
    │   │               └── FC3DArea()
    │   │               └── FC3DArea(const FC3DArea&)(const FC3DArea& other)
    │   │           └── 🗑️ ~FC3DArea()
    │   │           └── 📡 Публичные методы:
    │   │               └── используемый для арифметических операций(для согласованности с геометрическими примитивами)
    │   │               └── 1 с началом в(0, 0, 0)
    │   │               └── brief Конструктор с параметрами(базовый)
    │   │               └── param startPoint Начальная точка области(левый-нижний-ближний угол)
    │   │               └── param areaSize   Размер всей области(по умолчанию 1×1×1)
    │   │               └── param cellCounts Количество ячеек по осям(Nx, Ny, Nz)
    │   ├── FCCommand/
    │   │   └── FCCommand.h
    │   │       └── 📦 class FCCommand
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCCommand()
    │   │               └── FCCommand(const QString &gcodeLine)
    │   │           └── 📡 Публичные методы:
    │   │               └── < Геометрические команды и режимы(G0–G999)
    │   │               └── < Вспомогательные функции и управление(M0–M999)
    │   │               └── < Выбор активного экструдера(T0–T999)
    │   │           └── 🔢 Перечисления:
    │   │               └── enum Type
    │   │                   └── Unknown
    │   │                   └── ///< Неизвестная или некорректная команда.
        GCode
    │   │                   └── ///< Геометрические команды и режимы (G0–G999).
        MCode
    │   │                   └── ///< Вспомогательные функции и управление (M0–M999).
        TCode
    │   ├── FCConditionObject/
    │   │   ├── FCCondition/
    │   │   │   └── FCCondition.h
    │   │   │       └── 📦 class ReadyState
    │   │   │           └── 📌 Наследует: private ConditionType
    │   │   │       └── 📦 class PlayState
    │   │   │           └── 📌 Наследует: private ConditionType
    │   │   │       └── 📦 class ChangedState
    │   │   │           └── 📌 Наследует: private ConditionType
    │   │   │       └── 📦 class ErrorType
    │   │   │           └── 📌 Наследует: private ConditionType
    │   │   │       └── 📦 class PanelState
    │   │   │           └── 📌 Наследует: private ConditionType
    │   │   │       └── 📦 class VisibilityState
    │   │   │           └── 📌 Наследует: private ConditionType
    │   │   │       └── 📦 class FCCondition
    │   │   │           └── 🏗️ Конструкторы:
    │   │   │               └── FCCondition(ReadyState ready = ReadyState::NotReady, PlayState play = PlayState::Stop, ChangedState changed = ChangedState::NotChanged, ErrorType error = ErrorType::NoError, PanelState panel = PanelState::Main, VisibilityState visibility = VisibilityState::Hide)
    │   │   │               └── FCCondition(ReadyState ready)
    │   │   │               └── FCCondition(PlayState play)
    │   │   │               └── FCCondition(ChangedState changed)
    │   │   │               └── FCCondition(ErrorType error)
    │   │   │               └── FCCondition(PanelState panel, VisibilityState visibility)
    │   │   │               └── FCCondition(VisibilityState visibility)
    │   │   │           └── 📡 Публичные методы:
    │   │   │               └── param ready      Состояние готовности(по умолчанию NotReady)
    │   │   │               └── param play       Состояние воспроизведения(по умолчанию Stop)
    │   │   │               └── param changed    Флаг изменения(по умолчанию NotChanged)
    │   │   │               └── param error      Тип ошибки(по умолчанию NoError)
    │   │   │               └── param panel      Текущая панель интерфейса(по умолчанию Main)
    │   │   │               └── param visibility Состояние видимости(по умолчанию Hide)
    │   │   │               └── : _data((static_cast<uint16_t>(ready)
    │   │   │               └── brief Конструктор из состояния готовности(остальные поля = 0)
    │   │   │               └── : _data(static_cast<uint16_t>(ready)
    │   │   │               └── brief Конструктор из состояния воспроизведения(остальные поля = 0)
    │   │   │               └── : _data((static_cast<uint16_t>(play)
    │   │   │               └── brief Конструктор из флага изменения(остальные поля = 0)
    │   │   │               └── : _data((static_cast<uint16_t>(changed)
    │   │   │               └── brief Конструктор из типа ошибки(остальные поля = 0)
    │   │   │               └── : _data((static_cast<uint16_t>(error)
    │   │   │               └── brief Конструктор из состояния панели и видимости(остальные поля = 0)
    │   │   │               └── : _data(((static_cast<uint16_t>(panel)
    │   │   │               └── brief Конструктор из состояния видимости(остальные поля = 0)
    │   │   │               └── : _data((static_cast<uint16_t>(visibility)
    │   │   │               └── constexpr void set(ReadyState value) [noexcept]
    │   │   │           └── 💾 Публичные члены-данные:
    │   │   │               └── ReadyState ready = ReadyState::NotReady,
        PlayState play = PlayState::Stop,
        ChangedState changed = ChangedState::NotChanged,
        ErrorType error = ErrorType::NoError,
        PanelState panel = PanelState::Main,
        VisibilityState visibility = VisibilityState::Hide) noexcept
        : _data(
            (static_cast<uint16_t>(ready) & 0x03u) |
            ((static_cast<uint16_t>(play) & 0x03u) << 2) |
            ((static_cast<uint16_t>(changed) & 0x01u) << 4) |
            ((static_cast<uint16_t>(error) & 0x1Fu) << 5) |
            ((static_cast<uint16_t>(panel) & 0x03u) << 10) |
            ((static_cast<uint16_t>(visibility) & 0x01u) << 12)
          )
    {}

    /// @brief Конструктор из состояния готовности (остальные поля = 0).
    /// @param ready Состояние готовности.
    constexpr FCCondition(ReadyState ready) noexcept
        : _data(static_cast<uint16_t>(ready) & 0x03u)
    {}

    /// @brief Конструктор из состояния воспроизведения (остальные поля = 0).
    /// @param play Состояние воспроизведения.
    constexpr FCCondition(PlayState play) noexcept
        : _data((static_cast<uint16_t>(play) & 0x03u) << 2)
    {}

    /// @brief Конструктор из флага изменения (остальные поля = 0).
    /// @param changed Флаг изменения состояния.
    constexpr FCCondition(ChangedState changed) noexcept
        : _data((static_cast<uint16_t>(changed) & 0x01u) << 4)
    {}

    /// @brief Конструктор из типа ошибки (остальные поля = 0).
    /// @param error Тип ошибки.
    constexpr FCCondition(ErrorType error) noexcept
        : _data((static_cast<uint16_t>(error) & 0x1Fu) << 5)
    {}

    /// @brief Конструктор из состояния панели и видимости (остальные поля = 0).
    /// @param panel Текущая панель интерфейса.
    /// @param visibility Состояние видимости элемента.
    constexpr FCCondition(PanelState panel, VisibilityState visibility) noexcept
        : _data(
            ((static_cast<uint16_t>(panel) & 0x03u) << 10) |
            ((static_cast<uint16_t>(visibility) & 0x01u) << 12)
          )
    {}

    /// @brief Конструктор из состояния видимости (остальные поля = 0).
    /// @param visibility Состояние видимости элемента.
    constexpr FCCondition(VisibilityState visibility) noexcept
        : _data((static_cast<uint16_t>(visibility) & 0x01u) << 12)
    {}

    // --- сеттеры ---
    /// @brief Устанавливает новое состояние готовности.
    /// @param value Новое значение ReadyState.
    constexpr void set(ReadyState value) noexcept
    {
        _data = (_data & ~READY_MASK) | (static_cast<uint16_t>(value) & 0x03u)
    │   │   │           └── 🔑 Константы:
    │   │   │               └── uint16_t READY_MASK = 0x0003u
    │   │   │               └── uint16_t PLAY_MASK = 0x000Cu
    │   │   │               └── uint16_t CHANGED_MASK = 0x0010u
    │   │   │               └── uint16_t ERROR_MASK = 0x03E0u
    │   │   │               └── uint16_t PANEL_MASK = 0x0C00u
    │   │   │               └── uint16_t VISIBILITY_MASK = 0x1000u
    │   │   │               └── uint16_t RESERVED_MASK = 0xE000u
    │   │   └── FCConditionObject.h
    │   │       └── 📦 class FCConditionObject
    │   │           └── 📌 Наследует: public QObject
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCConditionObject(const QString &name, QObject *parent = nullptr) : QObject(parent)
    ...
    │   │           └── 📡 Публичные методы:
    │   │               └── param name Имя объекта(будет установлено через setObjectName()
    │   │               └── param parent Указатель на родительский QObject(для управления временем жизни)
    │   │               └── : QObject(parent)
    │   │               └──  set({ReadyState::Ready, PlayState::Stop, ChangedState::NotChanged, ErrorType::NoError})
    │   │               └──  setObjectName(name)
    │   │           └── 🔊 Сигналы:
    │   │               └── void conditionChanged(const QString &name, const FCCondition &condition)
    │   │       └── 📦 class FCConditionObjectList
    │   │           └── 📌 Наследует: public QList<T>
    │   │           └── @brief Конструктор с именованием объекта и установкой родителя.
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCConditionObjectList()
    │   │           └── 📡 Публичные методы:
    │   │               └── brief Конструктор по умолчанию(создаёт пустую коллекцию)
    │   │               └── note Пустая коллекция возвращает true(принцип универсального квантора: ∀x∈∅ P(x)
    │   │               └── bool is(ReadyState state) [const noexcept]
    │   │               └──  for(const auto &device : *this)
    │   │               └──  if(!device.is(state)
    │   │           └── 💾 Публичные члены-данные:
    │   │               └── если коллекция пуста ИЛИ все объекты имеют указанное состояние
    │   │               └── return false
    │   ├── FCRange/
    │   │   ├── build-FCRangeTest-Desktop_Qt_5_15_2_GCC_64bit-Debug/
    │   │   │   └── .qtc_clangd/
    │   │   │       └── .cache/
    │   │   │           └── clangd/
    │   │   │               └── index/
    │   │   └── FCRange.h
    │   │       └── 📦 class FCRange
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCRange(T min = defaultMin() [= default]
    │   │               └── FCRange(const FCRange&)
    │   │               └── FCRange()
    │   │               └── FCRange(const FCRange&)(const FCRange& other)
    │   │           └── 🗑️ ~FCRange()
    │   │           └── 📡 Публичные методы:
    │   │               └── static constexpr T defaultMin() [noexcept]
    │   │               └── if constexpr(std::is_floating_point_v<T>)
    │   ├── FCSpeed/
    │   │   ├── build-FC2DSpeedTest-Desktop_Qt_5_15_2_GCC_64bit-Debug/
    │   │   │   └── .qtc_clangd/
    │   │   │       └── .cache/
    │   │   │           └── clangd/
    │   │   │               └── index/
    │   │   └── FCSpeed.h
    │   │       └── 📦 class FCSpeed
    │   │           └── 🏗️ Конструкторы:
    │   │               └── FCSpeed()
    │   │               └── FCSpeed(_type move, _type draw)
    │   │               └── FCSpeed(const FCSpeed&)
    │   │               └── FCSpeed()
    │   │               └── FCSpeed(const FCSpeed&)(const FCSpeed& other)
    │   │           └── 🗑️ ~FCSpeed()
    │   │           └── 📡 Публичные методы:
    │   │               └── Тип данных для хранения значений скорости(32-битное знаковое целое)
    │   │               └── brief Возвращает нулевую скорость(движение = 0, отрисовка = 0)
    │   │               └── static constexpr FCSpeed zero() [noexcept]
    │   │               └── param move Скорость движения(линейная/угловая скорость механизма)
    │   │               └── param draw Скорость отрисовки(частота обновления графического вывода)
    │   │               └── : _move(move)
    │   │               └── brief Конструктор копирования(тривиальный)
    │   │               └── brief Оператор присваивания копированием(тривиальный)
    │   │               └── brief Деструктор(тривиальный)
    │   │               └── return Значение скорости движения(знаковое целое)
    │   │               └── note Отрицательное значение означает движение в обратном направлении(реверс)
    │   │               └── constexpr _type move() [const noexcept]
    │   │               └── return Значение скорости отрисовки(знаковое целое)
    │   │               └── note В контексте графики обычно положительное значение(частота кадров)
    │   │               └── constexpr _type draw() [const noexcept]
    │   │               └── constexpr void setMove(_type v) [noexcept]
    │   │               └── constexpr void setDraw(_type v) [noexcept]
    │   │               └── constexpr void set(_type move, _type draw) [noexcept]
    │   │           └── 💾 Публичные члены-данные:
    │   │               └── constexpr FCSpeed& operator [const] = (const FCSpeed&) noexcept = default
    │   │               └── return _move
    │   │               └── return _draw
    │   │               └──  _move = move
    │   │               └──  _draw = draw
    │   └── FCTransformations/
    │       └── FCCelsiusFahrenheit.h
    └── FCMachine.h
        └── 📦 class FCMachine
            └── 📌 Наследует: public QApplication
            └── 🏗️ Конструкторы:
                └── FCMachine(int argc, char **argv)
            └── 📡 Публичные методы:
                └── класса
    int exec()
```

*Корневая директория: FCMachine*
*Исключены папки: TEMP, TEST*
*Включены файлы: *.h*
*Извлечено: классы, наследование, конструкторы, методы, члены-данные, сигналы, слоты, перечисления, константы*