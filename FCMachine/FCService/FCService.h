#ifndef FC_SERVICE_H
#define FC_SERVICE_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QPointer>
#include <QString>

#include "FCState.h"

class FCService
    : public QObject
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCService)
public:
    using FCServiceState = FCStateT<FCReadyState, FCPlayState, FCErrorType>;

    explicit FCService(const QString &name, QObject *parent = nullptr)
        : QObject(parent)
    {
        setObjectName(name);
    }

    ~FCService() override = default;
};

using FCServiceList = QList<FCService*>;

#endif // FC_SERVICE_H
