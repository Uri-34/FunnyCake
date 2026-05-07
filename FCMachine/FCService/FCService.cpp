#include "FCService.h"

FCService::FCService(const QString &name, QObject *parent)
    : QObject(parent)
{
    setObjectName(name);
}
