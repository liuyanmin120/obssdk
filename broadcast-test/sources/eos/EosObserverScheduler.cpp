#include "EosObserverScheduler.h"
#include <QList>
#include <QThreadStorage>
#include <QSharedPointer>
#include <QThread>
#include <QDebug>

//--------------------------------------------------------------------------------
class EosGlobalScheduler
{
public:
    QMutex schedulersMutex;
    QList<GEosObserverScheduler *> schedulers;
};

//--------------------------------------------------------------------------------
//在这里，首先定义了一个EosGlobalScheduler，这个对象管理了多个线程局部存储的EosObserverScheduler
//也就是说eos系统只支持线程内部的通讯，不同的线程相同的names是无法连接在一起的。
Q_GLOBAL_STATIC(EosGlobalScheduler, theGlobalScheduler)
QThreadStorage<QSharedPointer<GEosObserverScheduler> > g_threadSchedulers;

//--------------------------------------------------------------------------------
//Dispatcher对象
GEosObserverDispatcher::GEosObserverDispatcher(QObject *parent)
    : QObject(parent)
{
}

void GEosObserverDispatcher::emitUpdatedSignalWithoutArgs()
{
    emit updated(QVariantList());
}

//--------------------------------------------------------------------------------
GEosObserverScheduler *theThreadScheduler()
{
    if (!g_threadSchedulers.hasLocalData())
    {
        QSharedPointer<GEosObserverScheduler> scheduler(new GEosObserverScheduler());
        g_threadSchedulers.setLocalData(scheduler);
    }

    return g_threadSchedulers.localData().data();
}

GEosObserverScheduler::GEosObserverScheduler()
{
    QMutexLocker globalLocker(&(theGlobalScheduler()->schedulersMutex));
    theGlobalScheduler()->schedulers.append(this);
}

GEosObserverScheduler::~GEosObserverScheduler()
{
    QMutexLocker globalLocker(&(theGlobalScheduler()->schedulersMutex));
    theGlobalScheduler()->schedulers.removeOne(this);
}

void GEosObserverScheduler::attach(const QObject *sender, const char *signal, const QString &name)
{
    GEosObserverDispatcher *dispatcher = subjectDispatcher(name, true);
    QObject::connect(sender, signal,
                     dispatcher, SLOT(emitUpdatedSignalWithoutArgs()),
                     Qt::UniqueConnection);
}

void GEosObserverScheduler::detach(const QObject *sender, const char *signal, const QString &name)
{
    GEosObserverDispatcher *dispatcher = subjectDispatcher(name, false);

    if (dispatcher)
    {
        QObject::disconnect(sender, signal,
                            dispatcher, SLOT(emitUpdatedSignalWithoutArgs()));
    }
}

void GEosObserverScheduler::attach(const QString &name, const QObject *receiver, const char *method)
{
    GEosObserverDispatcher *dispatcher = observerDispatcher(name, true);
   bool bRet = QObject::connect(dispatcher, SIGNAL(updated(QVariantList)),
                     receiver, method,
                     Qt::AutoConnection);
}

void GEosObserverScheduler::detach(const QString &name, const QObject *receiver, const char *method)
{
    GEosObserverDispatcher *dispatcher = observerDispatcher(name, false);

    if (dispatcher)
    {
        QObject::disconnect(dispatcher, SIGNAL(updated(QVariantList)),
                            receiver, method);
    }
}

void GEosObserverScheduler::notify(const QString &name, const QVariantList &args)
{
    subjectDispatcher(name, true)->updated(args);

#ifdef QT_DEBUG
    //qDebug("Thread[0x%08x]: '%s'", QThread::currentThreadId(), qPrintable(name));
#endif // QT_DEBUG
}

GEosObserverDispatcher *GEosObserverScheduler::subjectDispatcher(const QString &name, bool required)
{
    GEosObserverDispatcher *subjectDispatcher = m_subjectDispatchers.value(name);

    if (!subjectDispatcher && required)
    {
        subjectDispatcher = new GEosObserverDispatcher(this);
        m_subjectDispatchers.insert(name, subjectDispatcher);

        //检查内部subjectDispatcher和observerDispatcher中是否需要关联，若有建立起关联
        QMutexLocker globalLocker(&(theGlobalScheduler()->schedulersMutex));
        Q_FOREACH(GEosObserverScheduler * scheduler, theGlobalScheduler()->schedulers)
        {
            QMutexLocker locker(&scheduler->m_observerMutex);
            GEosObserverDispatcher *observerDispatcher
                = scheduler->m_observerDispatchers.value(name);

            if (observerDispatcher)
            {
                bool b = QObject::connect(subjectDispatcher, &GEosObserverDispatcher::updated,
                                 observerDispatcher, &GEosObserverDispatcher::updated);
				qDebug() << "Eos connect " << name << " :" << b;
            }
        }
    }

    return subjectDispatcher;
}

GEosObserverDispatcher *GEosObserverScheduler::observerDispatcher(const QString &name, bool required)
{
    GEosObserverDispatcher *observerDispatcher = m_observerDispatchers.value(name);

    if (!observerDispatcher && required)
    {
        observerDispatcher = new GEosObserverDispatcher(this);
        m_observerDispatchers.insert(name, observerDispatcher);

        //检查内部subjectDispatcher和observerDispatcher中是否需要关联，若有建立起关联
        QMutexLocker globalLocker(&(theGlobalScheduler()->schedulersMutex));
        Q_FOREACH(GEosObserverScheduler * scheduler, theGlobalScheduler()->schedulers)
        {
            QMutexLocker locker(&scheduler->m_subjectMutex);
            GEosObserverDispatcher *subjectDispatcher = scheduler->m_subjectDispatchers.value(name);

            if (subjectDispatcher)
            {
                QObject::connect(subjectDispatcher, &GEosObserverDispatcher::updated,
                                 observerDispatcher, &GEosObserverDispatcher::updated);
            }
        }
    }

    return observerDispatcher;
}
