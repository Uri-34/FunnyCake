#ifndef FC_GCODE_MAKER_H
#define FC_GCODE_MAKER_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QList>
#include <QMutex>
#include <QWaitCondition>
#include <QSharedPointer>
#include <QFile>
#include <QByteArray>

#include "FCState.h"
#include "FCImageBinaryContainer.h"
#include "FCSVGImageParser.h"

/**
 * @brief Класс кодирования и декодирования векторных изображений формата SVG.
 *
 * Предназначен для преобразования SVG-файлов во внутренний формат FCImageBinaryContainer
 * и обратно. Работает в отдельном потоке для предотвращения блокировок интерфейса
 * при обработке больших файлов.
 *
 * Ключевые возможности:
 *   - Парсинг SVG-файлов с поддержкой слоёв Inkscape,
 *   - Кодирование контейнеров обратно в SVG-формат,
 *   - Оптимизация путей (алгоритм Ramer-Douglas-Peucker),
 *   - Масштабирование под рабочую область плоттера,
 *   - Прогресс обработки через сигналы,
 *   - Потокобезопасная обработка через очередь команд.
 *
 * Архитектура многопоточности:
 *   - Работает в собственном экземпляре QThread (не наследует QThread!),
 *   - Взаимодействие через сигналы/слоты (Qt::QueuedConnection),
 *   - Все публичные методы неблокирующие — ставят задачу в очередь,
 *   - Состояние защищено мьютексом QMutex.
 *
 * @warning Критически важно:
 *          - Не вызывать публичные методы напрямую из рабочего потока,
 *          - При уничтожении объекта завершить поток через stopThread(),
 *          - SVG-файлы должны быть валидными (проверка через FCSVGImageParser).
 * @see FCSVGImageParser для низкоуровневого парсинга SVG
 * @see FCImageBinaryContainer для хранения распарсенных данных
 * @see FCPlotter для использования закодированных данных
 */
class FCGCodeMaker
    : public FCState
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCGCodeMaker)
public:
    // Константы (мс)
    static constexpr int CODER_THREAD_STOP_TIMEOUT_MS = 5000;      ///< Таймаут ожидания завершения потока
    static constexpr int CODER_COMMAND_PROCESS_INTERVAL_MS = 10;   ///< Интервал обработки команд
    static constexpr int CODER_PROGRESS_UPDATE_INTERVAL_MS = 100;  ///< Интервал обновления прогресса

    // Конструкторы / Деструктор
    /**
     * @brief Конструктор кодера с настройкой рабочего потока
     * @param name Имя устройства для идентификации в системе
     * @param parent Родительский QObject для управления временем жизни
     * @note При создании автоматически:
     *       - Создаётся и запускается рабочий поток QThread,
     *       - Объект перемещается в рабочий поток через moveToThread(),
     *       - Инициализируется парсер SVG (FCSVGImageParser).
     */
    explicit FCGCodeMaker(const QString &name, QObject *parent = nullptr);

    /**
     * @brief Деструктор кодера с корректным завершением рабочего потока
     * @note Автоматически выполняет:
     *       - Отправку команды остановки в рабочий поток,
     *       - Ожидание завершения всех операций (макс. 5 секунд),
     *       - Принудительное завершение потока при таймауте.
     * @warning Блокирует вызывающий поток на время ожидания (до 5 секунд).
     */
    ~FCGCodeMaker() override;

    // Управление потоком выполнения
    /**
     * @brief Запускает рабочий поток кодера
     * @return true при успешном запуске потока; иначе — false.
     */
    [[nodiscard]] bool startThread();

    /**
     * @brief Корректно останавливает рабочий поток кодера
     * @return true при корректном завершении; иначе — false.
     */
    [[nodiscard]] bool stopThread();

    // ========================================================================
    // Операции кодирования/декодирования (неблокирующие)
    // ========================================================================
    /**
     * @brief Запускает декодирование SVG-файла в контейнер
     * @param svgFilePath Путь к SVG-файлу для парсинга
     * @note Метод неблокирующий — задача ставится в очередь рабочего потока.
     */
    Q_INVOKABLE void decode(const QString &svgFilePath);

    /**
     * @brief Запускает кодирование контейнера в SVG-файл
     * @param container Контейнер с данными векторной модели
     * @param outputFilePath Путь для сохранения SVG-файла
     * @note Метод неблокирующий — задача ставится в очередь рабочего потока.
     */
    Q_INVOKABLE void encode(const FCImageBinaryContainer &container, const QString &outputFilePath);

    /**
     * @brief Отменяет текущую операцию кодирования/декодирования
     * @note Метод неблокирующий — отправляет команду отмены в рабочий поток.
     */
    Q_INVOKABLE void cancel();

    /**
     * @brief Проверяет, запущен ли рабочий поток кодера
     * @return true если поток активен и работает; иначе — false.
     */
    [[nodiscard]] bool isThreadRunning() const noexcept;

    /**
     * @brief Считывает секретный код привязки к оборудованию
     * @param timeoutMs Таймаут ожидания ответа (мс)
     * @return Код безопасности или пустая строка при ошибке.
     */
    [[nodiscard]] QString securityCode(int timeoutMs);

signals:
    /**
     * @brief Сигнал завершения декодирования
     * @param container Распарсенный контейнер с данными
     * @param success Результат операции (true = успешно)
     * @param details Детали результата (ошибки или статистика)
     */
    void decoded(const FCImageBinaryContainer &container, bool success, const QString &details);

    /**
     * @brief Сигнал завершения кодирования
     * @param outputPath Путь к сохранённому файлу
     * @param success Результат операции (true = успешно)
     * @param details Детали результата
     */
    void encoded(const QString &outputPath, bool success, const QString &details);

    /**
     * @brief Сигнал диагностического сообщения от кодера
     * @param name Имя источника сообщения
     * @param message Текст диагностического сообщения
     */
    void message(const QString &name, const QString &message);

    /**
     * @brief Сигнал прогресса обработки
     * @param percent Текущий прогресс в процентах (0–100)
     * @param stage Название текущего этапа обработки
     */
    void progress(int percent, const QString &stage);

private slots:
    /**
     * @brief Основной цикл выполнения рабочего потока
     * @note Выполняется ТОЛЬКО в рабочем потоке.
     */
    void run();

private:
    // Обработка команд
    // ... существующие методы ...
    /**
     * @brief Ожидать команду из очереди
     * @return Строка команды или пустая строка если таймаут
     * @note Блокирует поток на CODER_COMMAND_PROCESS_INTERVAL_MS
     */
    [[nodiscard]] QString waitForCommand();

    void processCommand(const QString &command);
    void processDecodeCommand();
    void processEncodeCommand();
    void processCancelCommand();

    // Операции кодирования/декодирования
    [[nodiscard]] bool performDecode(const QString &svgFilePath);
    [[nodiscard]] bool performEncode(const FCImageBinaryContainer &container, const QString &outputFilePath);

    // Инициализация и финализация
    [[nodiscard]] bool initializeParser();
    void finalizeParser();

    // Члены данных
    /// Рабочий поток для изоляции операций от основного потока интерфейса.
    QThread *_workerThread = nullptr;

    /// Мьютекс для синхронизации доступа к разделяемым ресурсам.
    mutable QMutex _mutex;

    /// Условие ожидания для блокировки рабочего потока при отсутствии команд.
    QWaitCondition _commandCondition;

    /// Очередь команд управления (decode/encode/cancel).
    QQueue<QString> _commandQueue;

    /// Путь к SVG-файлу для декодирования.
    QString _svgFilePath;

    /// Контейнер для кодирования в SVG.
    FCImageBinaryContainer _currentContainer;

    /// Путь для сохранения закодированного SVG-файла.
    QString _outputFilePath;

    /// Парсер SVG для низкоуровневой обработки.
    FCSVGImageParser *_parser = nullptr;

    /// Флаг запроса завершения рабочего потока.
    bool _stopRequested = false;

    /// Флаг отмены текущей операции.
    bool _cancelRequested = false;
};

// Псевдонимы типов
/// Список указателей на кодеры для управления группой устройств.
using FCGCodeMakerPtrList = QList<FCGCodeMaker*>;

#endif // FC_GCODE_MAKER_H
