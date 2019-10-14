#include "EosObserver.h"

#include "EosObserverScheduler.h"

void GEosObserver::attach(const QObject *sender, const char *signal, const char *name)
{
    theThreadScheduler()->attach(sender, signal, QString::fromLatin1(name));
}

void GEosObserver::detach(const QObject *sender, const char *signal, const char *name)
{
    theThreadScheduler()->detach(sender, signal, QString::fromLatin1(name));
}

void GEosObserver::attach(const char *name, const QObject *receiver, const char *method)
{
    theThreadScheduler()->attach(name, receiver, method);
}

void GEosObserver::detach(const char *name, const QObject *receiver, const char *method)
{
    theThreadScheduler()->detach(name, receiver, method);
}

void GEosObserver::notify(const char *name, const QVariantList &args)
{
    theThreadScheduler()->notify(QString::fromLatin1(name), args);
}

QSubject *QSubject::instance()
{
    static QSubject s_GInstance;
    return &s_GInstance;
}

void QSubject::attach(QObserver *obs)
{
    m_ls.append(obs);
}

void QSubject::detach(QObserver *obs)
{
    m_ls.removeAll(obs);
}

void QSubject::notify(const QVariant &arg0, const QVariant &arg1, const QVariant &arg2)
{
    QVariantList ls;
    ls << arg0 << arg1 << arg2;

    QObServerList::iterator it = m_ls.begin();

    for (; it != m_ls.end(); ++it)
    {
        (*it)->onNotify(ls);
    }
}
