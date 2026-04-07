#include "FCCommand.h"

FCCommand::FCCommand(const QString &gcodeLine)
{
    parse(gcodeLine);
}

bool FCCommand::parse(const QString &gcodeLine)
{
    *this = FCCommand(); // Сброс состояния

    if(gcodeLine.isEmpty())
    {
        return false;
    }

    QString line = gcodeLine.simplified();
    int pos = 0;

    // Пропуск начальных пробелов
    while(pos < line.length() && isWhitespace(line[pos]))
    {
        ++pos;
    }

    if(pos >= line.length())
    {
        return false;
    }

    // Парсинг номера строки (необязательно)
    if(line[pos].toUpper() == QLatin1Char('N'))
    {
        ++pos;
        int num = 0;
        int digits = 0;
        while(pos < line.length() && isDigit(line[pos]))
        {
            num = num * 10 + (line[pos].digitValue());
            ++pos;
            ++digits;
        }

        if(digits > 0)
        {
            _lineNumber = num;
            while(pos < line.length() && isWhitespace(line[pos]))
            {
                ++pos;
            }
        }
    }

    // Определение типа команды
    QChar cmdTypeChar = line[pos].toUpper();
    switch(cmdTypeChar.toLatin1())
    {
        case 'G': _type = GCode; break;
        case 'M': _type = MCode; break;
        case 'T': _type = TCode; break;
        default: return false;
    }
    ++pos;

    // Парсинг номера команды
    int cmdNum = 0;
    int digits = 0;
    while(pos < line.length() && isDigit(line[pos]))
    {
        cmdNum = cmdNum * 10 + (line[pos].digitValue());
        ++pos;
        ++digits;
    }

    if(digits == 0)
    {
        return false;
    }

    _number = cmdNum;

    // Парсинг параметров
    while(pos < line.length())
    {
        if(!isLetter(line[pos]))
        {
            break;
        }

        QChar letter = line[pos].toUpper();
        ++pos;

        // Пропуск пробелов
        while(pos < line.length() && isWhitespace(line[pos]))
        {
            ++pos;
        }

        if(pos >= line.length() || (!isDigit(line[pos]) && line[pos] != QLatin1Char('-') && line[pos] != QLatin1Char('.')))
        {
            break;
        }

        // Парсинг числа
        int numStart = pos;
        if(line[pos] == QLatin1Char('-') || line[pos] == QLatin1Char('+'))
        {
            ++pos;
        }

        while(pos < line.length() && (isDigit(line[pos]) || line[pos] == QLatin1Char('.')))
        {
            ++pos;
        }

        QString numStr = line.mid(numStart, pos - numStart);
        bool ok = false;
        float value = numStr.toFloat(&ok);
        if(ok)
        {
            _params[letter] = value;
        }

        // Пропуск пробелов после числа
        while(pos < line.length() && isWhitespace(line[pos]))
        {
            ++pos;
        }
    }

    // Обработка контрольной суммы и комментариев
    int checksumPos = line.indexOf(QLatin1Char('*'), pos);
    int semicolonPos = line.indexOf(QLatin1Char(';'), pos);
    int parenOpenPos = line.indexOf(QLatin1Char('('), pos);

    // Контрольная сумма должна быть до комментария
    if(checksumPos != -1 && (semicolonPos == -1 || checksumPos < semicolonPos) &&
        (parenOpenPos == -1 || checksumPos < parenOpenPos))
    {
        ++checksumPos;
        int cs = 0;
        int csDigits = 0;
        while(checksumPos < line.length() && isDigit(line[checksumPos]))
        {
            cs = cs * 10 + (line[checksumPos].digitValue());
            ++checksumPos;
            ++csDigits;
        }

        if(csDigits > 0)
        {
            _checksum = cs;
            if (!verifyChecksum(gcodeLine))
                _checksum.reset();
        }
    }

    // Извлечение комментария
    if(semicolonPos != -1)
    {
        _comment = line.mid(semicolonPos + 1).trimmed();
    }
    else if(parenOpenPos != -1)
         {
            int parenClosePos = line.indexOf(QLatin1Char(')'), parenOpenPos);
            if(parenClosePos != -1)
            {
                _comment = line.mid(parenOpenPos + 1, parenClosePos - parenOpenPos - 1).trimmed();
            }
         }

    return isValid();
}

bool FCCommand::hasParam(QChar letter) const noexcept
{
    return _params.contains(letter.toUpper());
}

std::optional<float> FCCommand::param(QChar letter) const noexcept
{
    auto it = _params.find(letter.toUpper());
    return (it != _params.end()) ? std::optional<float>(*it) : std::nullopt;
}

std::optional<int> FCCommand::paramInt(QChar letter) const noexcept
{
    auto val = param(letter);
    return val ? std::optional<int>(static_cast<int>(std::round(*val))) : std::nullopt;
}

std::optional<bool> FCCommand::paramBool(QChar letter) const noexcept
{
    auto val = param(letter);
    return val ? std::optional<bool>(*val != 0.0f) : std::nullopt;
}

void FCCommand::setParam(QChar letter, float value)
{
    _params[letter.toUpper()] = value;
}

void FCCommand::removeParam(QChar letter)
{
    _params.remove(letter.toUpper());
}

QChar FCCommand::typeChar() const noexcept
{
    switch(_type)
    {
        case GCode: return QLatin1Char('G');
        case MCode: return QLatin1Char('M');
        case TCode: return QLatin1Char('T');
        default: return QChar();
    }
}

QString FCCommand::typeString() const noexcept
{
    return QString(typeChar()) + QString::number(_number);
}

QString FCCommand::toString(bool withChecksum, bool withLineNumber) const
{
    if(!isValid())
    {
        return QString();
    }

    QString result;

    // Номер строки
    if(withLineNumber && _lineNumber.has_value())
    {
        result += QStringLiteral("N%1 ").arg(*_lineNumber);
    }
    // Команда
    result += typeString();

    // Параметры (QMap уже упорядочен)
    for(auto it = _params.constBegin(); it != _params.constEnd(); ++it)
    {
        result += QLatin1Char(' ');
        result += it.key();
        result += formatFloat(it.value());
    }

    // Комментарий
    if (!_comment.isEmpty())
    {
        if(_comment.contains(QLatin1Char('\n')) || _comment.startsWith(QLatin1Char('(')))
        {
            result += QStringLiteral(" (%1)").arg(_comment);
        }
        else
        {
            result += QStringLiteral(" ;%1").arg(_comment);
        }
    }

    // Контрольная сумма
    if (withChecksum)
    {
        int cs = computeChecksum(result);
        result += QStringLiteral("*%1").arg(cs);
    }

    return result;
}

bool FCCommand::isWhitespace(QChar c) noexcept
{
    return c.isSpace();
}

bool FCCommand::isDigit(QChar c) noexcept
{
    return c.isDigit();
}

bool FCCommand::isLetter(QChar c) noexcept
{
    return c.isLetter();
}

bool FCCommand::verifyChecksum(const QString &rawLine) const noexcept
{
    if(!_checksum.has_value())
    {
        return true;
    }

    int starPos = rawLine.indexOf(QLatin1Char('*'));
    if(starPos == -1)
    {
        return false;
    }

    int sum = 0;
    for(int i = 0; i < starPos; ++i)
    {
        QChar c = rawLine[i];
        if (!c.isSpace())
            sum ^= c.toLatin1();
    }

    return sum == *_checksum;
}

int FCCommand::computeChecksum(const QString &cmd) const noexcept
{
    int sum = 0;
    for(const QChar &c : cmd)
    {
        if(!c.isSpace())
        {
            sum ^= c.toLatin1();
        }
    }

    return sum;
}

QString FCCommand::formatFloat(float value)
{
    // Умное форматирование: целые без точки, остальные с 3 знаками после запятой
    if(std::fabs(value - std::round(value)) < 0.001f)
    {
        return QString::number(static_cast<int>(std::round(value)));
    }
    return QString::number(value, 'f', 3).replace(QLatin1Char('.'), QLatin1Char(',')).replace(QLatin1Char(','), QLatin1Char('.'));
}

QVariant FCCommand::toVariant() const
{
    QVariantMap map;
    map.insert(QStringLiteral("type"), static_cast<int>(_type));
    map.insert(QStringLiteral("number"), _number);
    if(_lineNumber.has_value())
    {
        map.insert(QStringLiteral("lineNumber"), *_lineNumber);
    }

    if (_checksum.has_value())
    {
        map.insert(QStringLiteral("checksum"), *_checksum);
    }

    map.insert(QStringLiteral("params"), QVariant::fromValue(_params));
    map.insert(QStringLiteral("comment"), _comment);
    return map;
}

FCCommand FCCommand::fromVariant(const QVariant &variant)
{
    FCCommand cmd;
    QVariantMap map = variant.toMap();

    cmd._type = static_cast<FCCommand::Type>(map.value(QStringLiteral("type"), 0).toInt());
    cmd._number = map.value(QStringLiteral("number"), -1).toInt();

    if(map.contains(QStringLiteral("lineNumber")))
    {
        cmd._lineNumber = map.value(QStringLiteral("lineNumber")).toInt();
    }

    if(map.contains(QStringLiteral("checksum")))
    {
        cmd._checksum = map.value(QStringLiteral("checksum")).toInt();
    }

    cmd._params = map.value(QStringLiteral("params")).value<QMap<QChar, float>>();
    cmd._comment = map.value(QStringLiteral("comment")).toString();

    return cmd;
}

//QDebug operator<<(QDebug dbg, const FCCommand &cmd)
//{
//    QDebugStateSaver saver(dbg);
//    dbg.nospace() << "FCCommand(";
//    if(!cmd.isValid())
//    {
//        dbg << "invalid)";
//        return dbg;
//    }

//    dbg << cmd.typeString();
//    for(auto it = cmd._params.constBegin(); it != cmd._params.constEnd(); ++it)
//    {
//        dbg << " " << it.key() << formatFloat(it.value());
//    }

//    if(!cmd._comment.isEmpty())
//    {
//        dbg << " ;" << cmd._comment;
//    }
//    dbg << ")";

//    return dbg;
//}

// Регистрация типа для QVariant (вызвать один раз в main())
#define REGISTER_FC_COMMAND_META_TYPE qRegisterMetaType<FCCommand>("FCCommand");
