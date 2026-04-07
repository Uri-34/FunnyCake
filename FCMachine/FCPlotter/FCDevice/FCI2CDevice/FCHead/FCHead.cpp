#include "FCHead.h"

void FCHead::selectColor(const QColor &color)
{
    // смена цвета это переключание между дюзами
    // - отвести все податчики в верхнее положение
    // - определить номер податчика с необходимым цветом
    // - переместить податчик определенного мотора в нижнее-рабочее положение
    if(removeAllFeeders())
    {
        if(submitFeeder(calculateFeeder(color)))
        {
            emit condition(state().set(FCReadyState::Ready, FCErrorType::None), objectName());
        }
    }
    else
    {
        emit condition(state().set(FCReadyState::NotReady, FCErrorType::Motion), objectName());
    }
}

QString FCHead::securityCode(int timeoutMs)
{
    Q_UNUSED(timeoutMs) // временная заглушка

    // прописать считывание секретного кода

    return {};
}

uint8_t FCHead::calculateFeeder(const QColor &color)
{
    for(int count = 0; count < _nozzles.size(); count++)
    {
        if(_nozzles[count].color() == color)
        {
            return count;
        }
    }

    return 0;
}

bool FCHead::removeAllFeeders()
{
    // прописать логику выполнения отвода всех податчиков в верхнее положение

    return true;
}

bool FCHead::submitFeeder(uint8_t number)
{
    Q_UNUSED(number) // временная заглушка

    // прописать логику выбранного податчика в рабочее-нижнее положение

    return true;
}

