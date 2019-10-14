#ifndef EOSOBSERVERSCHEDULER_H
#define EOSOBSERVERSCHEDULER_H

#include <QObject>
#include <QVariant>
#include <QObject>
#include <QMutex>
#include <QHash>

class GEosObserverDispatcher : public QObject
{
    Q_OBJECT
private:
    explicit GEosObserverDispatcher(QObject *parent = 0);

Q_SIGNALS:
    void updated(const QVariantList &args);

public Q_SLOTS:
    void emitUpdatedSignalWithoutArgs();

private:
    friend class GEosObserverScheduler;
};

class GEosObserverScheduler : public QObject
{
    friend GEosObserverScheduler *theThreadScheduler();

private:
    GEosObserverScheduler();
public:
    ~GEosObserverScheduler();

public:
    void attach(const QObject *sender, const char *signal, const QString &name);
    void detach(const QObject *sender, const char *signal, const QString &name);

    void attach(const QString &name, const QObject *receiver, const char *method);
    void detach(const QString &name, const QObject *receiver, const char *method);

    void notify(const QString &name, const QVariantList &args);

public:
    GEosObserverDispatcher *subjectDispatcher(const QString &name, bool required);
    GEosObserverDispatcher *observerDispatcher(const QString &name, bool required);

private:
    QMutex m_subjectMutex;
    QMutex m_observerMutex;

    QHash<QString, GEosObserverDispatcher *> m_subjectDispatchers;
    QHash<QString, GEosObserverDispatcher *> m_observerDispatchers;
};

GEosObserverScheduler *theThreadScheduler();

#endif // EOSOBSERVERSCHEDULER_H
