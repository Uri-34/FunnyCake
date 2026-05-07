#include "FCConfigFile.h"
#include <QDir>
#include <QStandardPaths>

// Единственный экземпляр класса создаётся при первом вызове instance()
// Используется идиома "Meyers Singleton" — потокобезопасно в C++11+
FCConfigFile& FCConfigFile::instance()
{
    static FCConfigFile _instance;
    return _instance;
}

// Приватный конструктор вызывается один раз при создании instance()
FCConfigFile::FCConfigFile()
{
//    // Проверяем, что QApplication или QCoreApplication уже созданы
//    // Без этого QCoreApplication::applicationFilePath() вернёт пустую строку
//    Q_ASSERT(qApp != nullptr && "QCoreApplication должен быть инициализирован до первого вызова FCConfigFile::instance()");

    // Формируем путь к конфигурационному файлу: <путь_к_исполняемому_файлу>.conf
    _confPath = QCoreApplication::applicationFilePath() + _configExtension;
    // Приводим путь к каноническому виду (удаляем ./, ../, двойные слеши и т.д.)
    _confPath = QDir::cleanPath(_confPath);

    // Загружаем настройки из файла (если файл существует) или создаём значения по умолчанию
    load();
}

// Загружает конфигурацию из INI-файла
void FCConfigFile::load()
{
    // Открываем конфигурационный файл в формате INI
    QSettings settings(_confPath, QSettings::IniFormat);

    // --- Загрузка пути к моделям ---
    // Формируем полный ключ: "paths/models"
    QString modelsKey = QStringLiteral("%1/%2").arg(KEY_PATHS, KEY_MODELS_PATH);
    QString loadedModelsPath = settings.value(modelsKey).toString();
    // Если путь не задан — используем папку "models" в домашней директории пользователя
    _modelsPath = loadedModelsPath.isEmpty() ? QDir::homePath() + QStringLiteral("/models") : loadedModelsPath;

    // --- Загрузка пути к кодам ---
    QString codesKey = QStringLiteral("%1/%2").arg(KEY_PATHS, KEY_CODES_PATH);
    QString loadedCodesPath = settings.value(codesKey).toString();
    _codesPath = loadedCodesPath.isEmpty() ? QDir::homePath() + QStringLiteral("/codes") : loadedCodesPath;

    // --- Загрузка списка серийных номеров плоттеров ---
    _plotterSerialNumbers = settings.value(KEY_SERIAL_NUMBERS).toStringList();



    // Если список пуст — можно инициализировать значениями по умолчанию
    // (например, для первого запуска программы)
    if (_plotterSerialNumbers.isEmpty())
    {
        // Тут добавить тестовые/резервные номера при формировании первичного конфига ...
        // Сейчас добавляем один так как ориентируемся на то что от одного компа работает один плоттер
        // В дальнейшем планируем несколько плоттеров
        _plotterSerialNumbers << "SN_TEST_000001";

        // Сохраняем конфигурацию, чтобы в следующий раз файл уже существовал
        save();
    }
}

// Сохраняет текущие настройки в конфигурационный файл
bool FCConfigFile::save()
{
    QSettings settings(_confPath, QSettings::IniFormat);

    // Сохраняем пути в группу "paths"
    settings.setValue(QStringLiteral("%1/%2").arg(KEY_PATHS, KEY_MODELS_PATH), _modelsPath);
    settings.setValue(QStringLiteral("%1/%2").arg(KEY_PATHS, KEY_CODES_PATH),  _codesPath);

    // Сохраняем список серийных номеров плоттеров
    settings.setValue(KEY_SERIAL_NUMBERS, _plotterSerialNumbers);

    // Принудительно сбрасываем буфер настроек на диск
    // (по умолчанию QSettings делает это при уничтожении, но явный вызов надёжнее)
    settings.sync();

    return true;
}

