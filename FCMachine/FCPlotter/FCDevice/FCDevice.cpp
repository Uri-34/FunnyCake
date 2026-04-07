#include "FCDevice.h"

FCDevice::FCDevice(const QString &name, QObject *parent)
: QObject(parent)
{
    setObjectName(name);
}
