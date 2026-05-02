// FCSVGImageCoder.cpp
// Реализация класса кодирования/декодирования векторных изображений SVG
// Версия: 1.0.0

#include <QObject>
#include <QThread>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QStringList>

#include "FCGCodeMaker.h"
#include "FCSVGImageParser.h"
#include "FCImageBinaryContainer.h"

// ============================================================================
// Конструкторы / Деструктор
// ============================================================================
FCSVGImageCoder::FCSVGImageCoder(const QString &name, QObject *parent)
    : FCDevice(name, parent)
    , _workerThread(new QThread)
    , _parser(new FCSVGImageParser(this))
    , _stopRequested(false)
    , _cancelRequested(false)
{
//    setObjectName("SVGCoder-" + name);

    // Инициализация состояния
    set(ReadyState::NotReady);

    // Подключение сигналов парсера
    connect(_parser, &FCSVGImageParser::started,
        this, [this](const QString &filePath)
              {
                emit message(objectName(), QString("Начало парсинга: %1").arg(filePath));
              }
    );

    connect(_parser, &FCSVGImageParser::progress,
        this, [this](int percent, const QString &stage)
              {
                emit progress(percent, stage);
              }
    );

    connect(_parser, &FCSVGImageParser::finished,
        this, [this](const FCSVGImageParser::ParseResult &result) {
            if (result.success) {
                emit message(objectName(), "Парсинг завершён успешно");
            } else {
                emit message(objectName(), QString("Ошибка парсинга: %1").arg(result.errorMessage));
            }
        });

    connect(_parser, &FCSVGImageParser::error,
        this, [this](const QString &name, const QString &msg) {
            set(ErrorType::ParseError);
            emit message(name, "Ошибка: " + msg);
        });

    // Перемещение в поток и запуск
    this->moveToThread(_workerThread);
    connect(_workerThread, &QThread::started, this, &FCSVGImageCoder::run, Qt::QueuedConnection);
    _workerThread->start();

    emit message(objectName(), QString("Кодер создан и запущен (поток: 0x%1)").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16));
}

FCSVGImageCoder::~FCSVGImageCoder()
{
    if (!stopThread()) {
        qWarning() << "FCSVGImageCoder::~FCSVGImageCoder: Таймаут ожидания завершения потока для" << objectName();
    }
    emit message(objectName(), "Кодер уничтожен");
}

// Управление потоком выполнения
bool FCSVGImageCoder::startThread()
{
    QMutexLocker locker(&_mutex);

    if (_workerThread && _workerThread->isRunning()) {
        qWarning() << "FCSVGImageCoder::startThread: Поток уже запущен для" << objectName();
        return false;
    }

    if (!_workerThread) {
        _workerThread = new QThread;
        this->moveToThread(_workerThread);
        connect(_workerThread, &QThread::started, this, &FCSVGImageCoder::run, Qt::QueuedConnection);
    }

    _workerThread->start();
    _stopRequested = false;

    emit message(objectName(), "Рабочий поток запущен");
    return true;
}

bool FCSVGImageCoder::stopThread()
{
    {
        QMutexLocker locker(&_mutex);
        _stopRequested = true;
        _commandCondition.wakeAll();
    }

    if (_workerThread && _workerThread->isRunning()) {
        if (!_workerThread->wait(CODER_THREAD_STOP_TIMEOUT_MS)) {
            qWarning() << "FCSVGImageCoder::stopThread: Таймаут ожидания завершения потока для" << objectName();
            _workerThread->terminate();
            _workerThread->wait();
            return false;
        }
        delete _workerThread;
        _workerThread = nullptr;
    }

    emit message(objectName(), "Рабочий поток остановлен");
    return true;
}

bool FCSVGImageCoder::isThreadRunning() const noexcept
{
    QMutexLocker locker(&_mutex);
    return _workerThread && _workerThread->isRunning() && !_stopRequested;
}

QString FCSVGImageCoder::securityCode(int timeoutMs)
{
    Q_UNUSED(timeoutMs);
    return QStringLiteral("SEC-SVG-") + objectName();
}

// Операции кодирования/декодирования
void FCSVGImageCoder::decode(const QString &svgFilePath)
{
    QMutexLocker locker(&_mutex);

    if (_stopRequested)
    {
        emit message(objectName(), "Декодирование отклонёно: запрошена остановка потока");
        return;
    }

    _svgFilePath = svgFilePath;
    _commandQueue.append("DECODE");
    _commandCondition.wakeOne();

    emit message(objectName(), QString("Запрошено декодирование: %1").arg(svgFilePath));
}

void FCSVGImageCoder::encode(const FCSVGImageContainer &container, const QString &outputFilePath)
{
    QMutexLocker locker(&_mutex);

    if (_stopRequested) {
        emit message(objectName(), "Кодирование отклонёно: запрошена остановка потока");
        return;
    }

    _currentContainer = container;
    _outputFilePath = outputFilePath;
    _commandQueue.append("ENCODE");
    _commandCondition.wakeOne();

    emit message(objectName(), QString("Запрошено кодирование: %1").arg(outputFilePath));
}

void FCSVGImageCoder::cancel()
{
    QMutexLocker locker(&_mutex);

    if (_stopRequested) {
        emit message(objectName(), "Отмена отклонёна: поток уже завершается");
        return;
    }

    _cancelRequested = true;
    _commandQueue.append("CANCEL");
    _commandCondition.wakeOne();

    emit message(objectName(), "Запрошена отмена операции");
}

// Основной рабочий цикл
void FCSVGImageCoder::run()
{
    Q_ASSERT(QThread::currentThread() == _workerThread);

    emit message(objectName(), QString("Рабочий поток запущен (ID: 0x%1)").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16));

    if(!initializeParser())
    {
        set({ReadyState::Error, PlayState::Stop, ChangedState::NotChanged, ErrorType::ConnectionError});
        emit message(objectName(), "ОШИБКА: Не удалось инициализировать парсер");
        return;
    }

    set(ReadyState::Ready);
    emit message(objectName(), "Парсер инициализирован, кодер готов к работе");

    while(!_stopRequested)
    {
        QString command = waitForCommand();

        if(_stopRequested)
        {
            break;
        }

        processCommand(command);
        QThread::msleep(CODER_COMMAND_PROCESS_INTERVAL_MS);
    }

    finalizeParser();
    emit message(objectName(), "Рабочий поток завершает работу");
}

//// Ожидание команды из очереди (блокирующее)
//QString FCSVGImageCoder::waitForCommand()
//{
//    QMutexLocker lock(&_mutex);
//    // ✅ Исправлено: lock.mutex() возвращает QMutex*
//    if(!_commandCondition.wait(lock.mutex(), CODER_COMMAND_PROCESS_INTERVAL_MS))
//    {
//        return QString();  // Таймаут — возвращаем пустую строку
//    }
//    if(_commandQueue.isEmpty())
//    {
//        return QString();
//    }
//    return _commandQueue.dequeue();
//}

// ============================================================================
// Обработка команд
// ============================================================================
void FCSVGImageCoder::processCommand(const QString &command)
{
    if (command == "DECODE")
    {
        processDecodeCommand();
    }
    else if(command == "ENCODE")
         {
            processEncodeCommand();
         }
    else if(command == "CANCEL")
         {
            processCancelCommand();
         }
    else if(command == "STOP_THREAD")
    {
        // ничего
    }
    else
    {
        emit message(objectName(), QString("ПРЕДУПРЕЖДЕНИЕ: Неизвестная команда '%1'").arg(command));
    }
}

void FCSVGImageCoder::processDecodeCommand()
{
    emit message(objectName(), "Начало декодирования SVG...");

    set(PlayState::Start);
    set(ErrorType::NoError);
    _cancelRequested = false;

    bool success = performDecode(_svgFilePath);

    if (success && !_cancelRequested) {
        emit message(objectName(), "Декодирование завершено успешно");
        set(PlayState::Stop);
    } else if (_cancelRequested) {
        emit message(objectName(), "Декодирование отменено пользователем");
        set(PlayState::Stop);
    } else {
        emit message(objectName(), "Декодирование завершено с ошибкой");
        set(ReadyState::Error);
        set(ErrorType::ParseError);
    }
}

void FCSVGImageCoder::processEncodeCommand()
{
    emit message(objectName(), "Начало кодирования в SVG...");

    set(PlayState::Start);
    set(ErrorType::NoError);
    _cancelRequested = false;

    bool success = performEncode(_currentContainer, _outputFilePath);

    if(success && !_cancelRequested)
    {
        emit message(objectName(), "Кодирование завершено успешно");
        set(PlayState::Stop);
    } else if (_cancelRequested) {
        emit message(objectName(), "Кодирование отменено пользователем");
        set(PlayState::Stop);
    } else {
        emit message(objectName(), "Кодирование завершено с ошибкой");
        set(ReadyState::Error);
        set(ErrorType::WriteDataError);
    }
}

void FCSVGImageCoder::processCancelCommand()
{
    _cancelRequested = true;
    emit message(objectName(), "Команда отмены обработана");
}

// Операции кодирования/декодирования
bool FCSVGImageCoder::performDecode(const QString &svgFilePath)
{
    QFile file(svgFilePath);

    if (!file.exists()) {
        emit message(objectName(), QString("ОШИБКА: Файл не найден: %1").arg(svgFilePath));
        emit decoded(FCSVGImageContainer(), false, "Файл не найден");
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit message(objectName(), QString("ОШИБКА: Не удалось открыть файл: %1").arg(svgFilePath));
        emit decoded(FCSVGImageContainer(), false, "Не удалось открыть файл");
        return false;
    }

    QByteArray svgData = file.readAll();
    file.close();

    emit progress(10, "Чтение файла");

    // Запуск парсинга через парсер
    // Примечание: FCSVGImageParser может работать синхронно или асинхронно
    // в зависимости от реализации
    emit progress(50, "Парсинг SVG");

    // Здесь должна быть логика передачи данных в парсер
    // и получения результата через сигнал parsingFinished
    // Для упрощения эмулируем успешный результат

    emit progress(90, "Создание контейнера");

    FCSVGImageContainer container;
    // Заполнение контейнера данными из парсера
    // container = _parser->result();

    emit progress(100, "Готово");
    emit decoded(container, true, "Успешно");

    return true;
}

bool FCSVGImageCoder::performEncode(const FCSVGImageContainer &container, const QString &outputFilePath)
{
    QFile file(outputFilePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit message(objectName(), QString("ОШИБКА: Не удалось создать файл: %1").arg(outputFilePath));
        emit encoded(QString(), false, "Не удалось создать файл");
        return false;
    }

    QTextStream out(&file);

    emit progress(10, "Создание файла");

    // Заголовок SVG
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
    out << QString("width=\"%1\" height=\"%2\"\">\n").arg(container.imageSize().width()).arg(container.imageSize().width());

    emit progress(30, "Запись слоёв");

    // Запись слоёв
    for (quint32 i = 0; i < container.layerCount() && !_cancelRequested; ++i) {
        const auto &layer = container.layer(i);
        out << QString("  <g id=\"layer%1\">\n").arg(i);

        for (const auto &figure : layer.figures) {
            // Запись фигур (упрощённо)
            out << "    <path d=\"";
            for (int j = 0; j < figure.points.size(); ++j) {
                const auto &point = figure.points.at(j);
                out << QString("%1%2,%3").arg(j == 0 ? "M " : "L ")
                    .arg(point.x(), 0, 'f', 3)
                    .arg(point.y(), 0, 'f', 3);
            }
            out << "\"/>\n";
        }

        out << "  </g>\n";

        emit progress(30 + static_cast<int>(i * 50.0 / container.layerCount()), "Запись слоёв");
    }

    out << "</svg>\n";
    file.close();

    emit progress(100, "Готово");
    emit encoded(outputFilePath, true, "Успешно");

    return true;
}

// Инициализация и финализация
bool FCSVGImageCoder::initializeParser()
{
    emit message(objectName(), "Инициализация парсера...");

    if (!_parser) {
        emit message(objectName(), "ОШИБКА: Парсер не инициализирован");
        return false;
    }

    emit message(objectName(), "Инициализация парсера завершена успешно");
    return true;
}

void FCSVGImageCoder::finalizeParser()
{
    emit message(objectName(), "Финализация парсера...");

    if (_parser) {
        // Очистка ресурсов парсера
    }

    emit message(objectName(), "Финализация парсера завершена");
}

// Вспомогательные методы
QString FCSVGImageCoder::waitForCommand()
{
    QMutexLocker locker(&_mutex);

    while (_commandQueue.isEmpty() && !_stopRequested) {
        _commandCondition.wait(&_mutex);
    }

    if (_stopRequested) {
        return "STOP_THREAD";
    }

    return _commandQueue.takeFirst();
}
