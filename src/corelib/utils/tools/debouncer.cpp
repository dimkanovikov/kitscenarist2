#include "debouncer.h"


Debouncer::Debouncer(int _delayInMs, QObject* _parent)
    : QObject(_parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(_delayInMs);

    QObject::connect(&m_timer, &QTimer::timeout, this, [this] { emit gotWork(); });
}

void Debouncer::orderWork()
{
    if (!m_timer.isActive()) {
        emit pendingWork();
    }
    m_timer.start();
}

void Debouncer::abortWork()
{
    m_timer.stop();
}
