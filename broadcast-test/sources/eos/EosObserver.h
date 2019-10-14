#ifndef EOSOBSERVER_H
#define EOSOBSERVER_H

#include <QObject>
#include <QVariant>

class GEosObserver
{
public:
    static void attach(const QObject *sender, const char *signal, const char *name);
    static void detach(const QObject *sender, const char *signal, const char *name);

    static void attach(const char *name, const QObject *receiver, const char *method);
    static void detach(const char *name, const QObject *receiver, const char *method);

public:
    static void notify(const char *name, const QVariantList &args = QVariantList());

public:
    static inline void notify(const char *name, const QVariant &arg0);
    static inline void notify(const char *name, const QVariant &arg0,const QVariant &arg1);
    static inline void notify(const char *name, const QVariant &arg0,const QVariant &arg1, const QVariant &arg2);
    static inline void notify(const char *name, const QVariant &arg0,const QVariant &arg1, const QVariant &arg2,const QVariant &arg3);
    static inline void notify(const char *name, const QVariant &arg0,const QVariant &arg1, const QVariant &arg2, const QVariant &arg3, const QVariant &arg4);
    static inline void notify(const char *name, const QVariant &arg0,const QVariant &arg1, const QVariant &arg2,const QVariant &arg3, const QVariant &arg4, const QVariant &arg5);
};

inline void GEosObserver::notify(const char *name, const QVariant &arg0)
{
    GEosObserver::notify(name, QVariantList() << arg0);
}

inline void GEosObserver::notify(const char *name, const QVariant &arg0, const QVariant &arg1)
{
    GEosObserver::notify(name, QVariantList() << arg0 << arg1);
}

inline void GEosObserver::notify(const char *name, const QVariant &arg0, const QVariant &arg1, const QVariant &arg2)
{
    GEosObserver::notify(name, QVariantList() << arg0 << arg1 << arg2);
}

inline void GEosObserver::notify(const char *name, const QVariant &arg0, const QVariant &arg1, const QVariant &arg2,const QVariant &arg3)
{
    GEosObserver::notify(name, QVariantList() << arg0 << arg1 << arg2 << arg3);
}

inline void GEosObserver::notify(const char *name, const QVariant &arg0, const QVariant &arg1, const QVariant &arg2,const QVariant &arg3, const QVariant &arg4)
{
    GEosObserver::notify(name,QVariantList() << arg0 << arg1 << arg2 << arg3 << arg4);
}

inline void GEosObserver::notify(const char *name, const QVariant &arg0, const QVariant &arg1, const QVariant &arg2,const QVariant &arg3, const QVariant &arg4,const QVariant &arg5)
{
    GEosObserver::notify(name, QVariantList() << arg0 << arg1 << arg2 << arg3 << arg4 << arg5);
}

class QObserver
{
public:
    virtual void onNotify(const QVariantList &datalist) = 0;
};

class QSubject
{
    typedef QList<QObserver*> QObServerList;
public:
    static QSubject * instance();

    void attach(QObserver * obs);
    void detach(QObserver * obs);

    void notify(const QVariant &arg0, const QVariant &arg1, const QVariant &arg2);

private:
    QObServerList m_ls;
};

template <typename T>
class QObServerT : public QObserver
{
    typedef void (T::*FUNC)(const QVariantList &datalist);
public:
    QObServerT(T * obj,FUNC func):m_obj(obj),m_func(func)
    {
        QSubject::instance()->attach(this);
    }
    ~QObServerT()
    {
        QSubject::instance()->detach(this);
    }

    virtual void onNotify(const QVariantList &datalist)
    {
        if(m_obj && m_func)
        {
            (m_obj->*m_func)(datalist);
        }
    }

private:
    T * m_obj;
    FUNC m_func;
};

#endif // EOSOBSERVER_H
