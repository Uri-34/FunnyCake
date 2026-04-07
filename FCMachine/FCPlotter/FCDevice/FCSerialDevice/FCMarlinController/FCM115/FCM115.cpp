#include "FCM115.h"
#include <QStringList>
#include <QRegularExpression>

#include "FC3DArea.h"

// ============================================================================
// РЕГИСТРАЦИЯ МЕТАТИПА ДЛЯ ИСПОЛЬЗОВАНИЯ В СИГНАЛАХ/СЛОТАХ
// ============================================================================
// Должно быть вызвано ОДИН РАЗ при старте программы (обычно в main.cpp):
// qRegisterMetaType<FCM115>("FCM115");
// ============================================================================

bool FCM115::parse(const QString &response)
{
    if(response.isEmpty())
    {
        _isValid = false;
        return false;
    }

    // Извлечение основных полей
    _firmware = extractField(response, "FIRMWARE_NAME");
    _version = extractField(response, "PROTOCOL_VERSION");
    _uuid = extractField(response, "UUID");

    // Проверка обязательных полей
    if(_firmware.isEmpty() || _version.isEmpty() || _uuid.isEmpty())
    {
        _isValid = false;
        return false;
    }

    // Извлечение и парсинг MACHINE_TYPE
    QString machineTypeStr = extractField(response, "MACHINE_TYPE");
    if(!parseMachineType(machineTypeStr))
    {
        _isValid = false;
        return false;
    }

    _isValid = true;
    return true;
}

bool FCM115::parseMachineType(const QString &machineTypeStr)
{
    if(machineTypeStr.isEmpty())
    {
        return false;
    }

    // Разделение MACHINE_TYPE на подполя по ';'
    // Формат: "NAME:FCv0.7;AREA:650.0,500.0,20.0,4,4;SPEED:15000,15000;KINEMATICS:Cartesian"
    QStringList parts = machineTypeStr.split(';', Qt::SkipEmptyParts);

    for(const QString &part : parts)
    {
        int colonPos = part.indexOf(':');
        if(colonPos == -1)
        {
            continue; // Пропускаем некорректные части
        }

        QString key = part.left(colonPos).trimmed();
        QString value = part.mid(colonPos + 1).trimmed();

        if(key == "NAME")
        {
            _machine = value;
        }
        else if(key == "AREA")
        {
            // Формат: "шир,глуб,выс,минZ,максZ"
            QStringList areaParts = value.split(',');
            if(areaParts.size() >= 5)
            {
                bool okWidth, okDepth, okHeight, okMinZ, okMaxZ;
                qreal width = areaParts[0].toDouble(&okWidth);
                qreal depth = areaParts[1].toDouble(&okDepth);
                qreal height = areaParts[2].toDouble(&okHeight);
                qreal minZ = areaParts[3].toDouble(&okMinZ);
                qreal maxZ = areaParts[4].toDouble(&okMaxZ);

                if(okWidth && okDepth && okHeight && okMinZ && okMaxZ)
                {
                    _area = FC3DArea(width, depth, height, minZ, maxZ);
                }
                else
                {
                    return false;
                }
            }
            else {
                    return false;
                 }
        }
        else if (key == "SPEED") {
            // Формат: "скоростьX,скоростьY" (в мм/мин)
            QStringList speedParts = value.split(',');
            if (speedParts.size() >= 2) {
                bool okX, okY;
                qreal speedX = speedParts[0].toDouble(&okX);
                qreal speedY = speedParts[1].toDouble(&okY);

                if (okX && okY) {
                    _speeds = FCSpeed(speedX, speedY);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        // KINEMATICS и другие поля игнорируем — не используются в текущей архитектуре
    }

    // Проверка, что все обязательные подполя MACHINE_TYPE извлечены
    return !_machine.isEmpty();
}

QString FCM115::extractField(const QString &response, const QString &key)
{
    // Поиск ключа в строке ответа
    int keyPos = response.indexOf(key + ":");
    if (keyPos == -1) {
        return QString();
    }

    // Начало значения (после "КЛЮЧ:")
    int valueStart = keyPos + key.length() + 1;

    // Поиск конца значения (до следующего ключа или конца строки)
    // Следующий ключ начинается с заглавной буквы и содержит ":"
    int valueEnd = response.length();

    // Ищем следующий ключ в формате "KEY:" (заглавные буквы + двоеточие)
    QRegularExpression nextKeyPattern("([A-Z_]+:)");
    QRegularExpressionMatch match = nextKeyPattern.match(response, valueStart);

    if (match.hasMatch()) {
        valueEnd = match.capturedStart();
    }

    // Извлечение значения и удаление лишних пробелов
    QString value = response.mid(valueStart, valueEnd - valueStart).trimmed();

    return value;
}

// ============================================================================
// РЕГИСТРАЦИЯ МЕТАТИПА (вызывается ОДИН РАЗ в программе)
// ============================================================================
// В файле, где используется класс (например, в FCMarlinController.cpp или в main.cpp):
//
// #include "FCM115.h"
//
// int main(int argc, char *argv[])
// {
//     QApplication app(argc, argv);
//
//     // Регистрация метатипа для использования в сигналах/слотах
//     qRegisterMetaType<FCM115>("FCM115");
//
//     // ... остальной код ...
//     return app.exec();
// }
// ============================================================================
