#include "FCPlotterCommandQueue.h"

FCPlotterCommandQueue::FCPlotterCommandQueue(QObject *parent)
    : QObject(parent)
    , _stopRequested(false)
{
}

void FCPlotterCommandQueue::enqueue(const QString &command)
{
    QMutexLocker locker(&_mutex);
    if (_stopRequested) {
        return;
    }
    _queue.append(command);
    _condition.wakeOne();
    emit commandAvailable();
}

QString FCPlotterCommandQueue::dequeue()
{
    QMutexLocker locker(&_mutex);
    if (_queue.isEmpty()) {
        return QString();
    }
    return _queue.takeFirst();
}

QString FCPlotterCommandQueue::waitForCommand(int timeoutMs)
{
    QMutexLocker locker(&_mutex);
    while (_queue.isEmpty() && !_stopRequested) {
        if (timeoutMs > 0) {
            _condition.wait(&_mutex, timeoutMs);
        } else {
            _condition.wait(&_mutex);
        }
    }

    if (_stopRequested) {
        return QStringLiteral("STOP_THREAD");
    }

    return _queue.takeFirst();
}

bool FCPlotterCommandQueue::isEmpty() const noexcept
{
    QMutexLocker locker(&_mutex);
    return _queue.isEmpty();
}

int FCPlotterCommandQueue::size() const noexcept
{
    QMutexLocker locker(&_mutex);
    return _queue.size();
}

void FCPlotterCommandQueue::clear()
{
    QMutexLocker locker(&_mutex);
    _queue.clear();
}

void FCPlotterCommandQueue::requestStop()
{
    QMutexLocker locker(&_mutex);
    _stopRequested = true;
    _condition.wakeAll();
    emit stopRequested();
}

bool FCPlotterCommandQueue::stopRequested() const noexcept
{
    QMutexLocker locker(&_mutex);
    return _stopRequested;
}
