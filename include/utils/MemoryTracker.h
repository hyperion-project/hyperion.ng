#ifndef MEMORYTRACKER_H
#define MEMORYTRACKER_H

#include <typeinfo>
#include <type_traits>
#include <memory>

#include <QSharedPointer>
#include <QObject>
#include <QThread>
#include <QString>
#include <QDebug>
#include <QVariant>
#include <QLoggingCategory>

#include <utils/global_defines.h>

// Base categories used when no explicit category passed.
Q_DECLARE_LOGGING_CATEGORY(hyperion_objects_track)

// Dynamic category variants (pass any QLoggingCategory expression returning const QLoggingCategory&)
#define TRACK_SCOPE_CATEGORY(category) \
    qCDebug(category).noquote() \
        << QString("%1()...").arg(__func__)
#define TRACK_SCOPE_SUBCOMPONENT_CATEGORY(category) \
    qCDebug(category).noquote() \
        << QString("|%1| %2()...").arg(_log->getSubName(), __func__)
#define MAKE_TRACKED_SHARED_CAT(T, catExpr, ...) makeTrackedShared<T>(this, (catExpr), __VA_ARGS__)

#define TRACK_SCOPE() \
    TRACK_SCOPE_CATEGORY(hyperion_objects_track)
#define TRACK_SCOPE_SUBCOMPONENT() \
    TRACK_SCOPE_SUBCOMPONENT_CATEGORY(hyperion_objects_track)
#define MAKE_TRACKED_SHARED(T, ...) makeTrackedShared<T>(this, hyperion_objects_track(), __VA_ARGS__)

// Internal helper macro: unified instance creation with dynamic category and variadic ctor args.
#define _MT_CREATE(storageRef, Type, creatorObj, catExpr, ...)                                                                                        \
    do                                                                                                                                                \
    {                                                                                                                                                 \
        if ((storageRef).isNull())                                                                                                                    \
        {                                                                                                                                             \
            std::unique_ptr<Type> _mt_up(new Type(__VA_ARGS__));                                                                                      \
            Type *_mt_raw = _mt_up.get();                                                                                                             \
            QString _mt_sub = "__";                                                                                                                   \
            QString _mt_tn = _mt_raw->metaObject()->className();                                                                                      \
            QObject *_mt_creator_ptr = static_cast<QObject *>(creatorObj);                                                                            \
            QString _mt_creator = (_mt_creator_ptr) ? QString::fromLatin1(_mt_creator_ptr->metaObject()->className()) : QStringLiteral("non-parent"); \
            const QLoggingCategory &_mt_cat = (catExpr);                                                                                              \
            qCDebug(_mt_cat).noquote()                                                                                                                \
                << QString("|%1| Creating object of type '%2' by %3 - [%4]'")                                                                         \
                       .arg(_mt_sub, _mt_tn, _mt_creator,                                                                                             \
                            QString("0x%1").arg(reinterpret_cast<quintptr>(_mt_raw), QT_POINTER_SIZE * 2, 16, QChar('0')));                           \
            auto _mt_del = [_mt_sub, _mt_tn, &_mt_cat](Type *ptr) { objectDeleter<Type>(ptr, _mt_sub, _mt_tn, _mt_cat); };                            \
            (storageRef) = QSharedPointer<Type>(_mt_raw, _mt_del);                                                                                    \
            _mt_up.release();                                                                                                                         \
        }                                                                                                                                             \
    } while (0)

// Public convenience macros
#define CREATE_INSTANCE_WITH_TRACKING(storageRef, Type, creatorObj, ...) \
    _MT_CREATE(storageRef, Type, creatorObj, hyperion_objects_track(), __VA_ARGS__)

#define CREATE_INSTANCE_WITH_TRACKING_CAT(storageRef, Type, creatorObj, catExpr, ...) \
    _MT_CREATE(storageRef, Type, creatorObj, (catExpr), __VA_ARGS__)

template<typename T>
void objectDeleter(T* ptr, const QString& subComponent, const QString& typeName, const QLoggingCategory& category)
{
    if (!ptr)
    {
        return;
    }

    QString const addr = QString("0x%1").arg(reinterpret_cast<quintptr>(ptr), QT_POINTER_SIZE * 2, 16, QChar('0'));

    if constexpr (std::is_base_of_v<QObject, T>)
    {
        QThread const *thread = ptr->thread();
        if (thread && thread == QThread::currentThread())
        {
            qCDebug(category).noquote() << QString("|%1| QObject<%2> deleted immediately (owning thread) - [%3]").arg(subComponent, typeName).arg(addr);
            delete ptr;
        }
        else if (thread && thread->isRunning())
        {
            qCDebug(category).noquote() << QString("|%1| QObject<%2>::deleteLater() scheduled on thread '%3' - [%4]")
                                               .arg(subComponent, typeName)
                                               .arg(thread->objectName())
                                               .arg(addr);
            QMetaObject::invokeMethod(ptr, "deleteLater", Qt::QueuedConnection);
        }
        else
        {
            qCDebug(category).noquote() << QString("|%1| QObject<%2> immediate delete (owning thread not running) - [%3]").arg(subComponent, typeName, addr);
            delete ptr; // last resort: thread ended
        }
    }
    else
    {
        qCDebug(category).noquote() << QString("|%1| Non-QObject<%2> deleted immediately - [%3]").arg(subComponent, typeName).arg(addr);
        delete ptr;
    }
}

// Factory function template to create tracked QSharedPointer
template<typename T, typename Creator, typename... Args>
QSharedPointer<T> makeTrackedShared(Creator creator, const QLoggingCategory& category, Args&&... args)
{
    auto* rawPtr = new T(std::forward<Args>(args)...);
    if (!rawPtr)
    {
        return QSharedPointer<T>();
    }

    QString subComponent = "__";
    QString typeName;
    if constexpr (std::is_base_of_v<QObject, T>)
    {
        typeName = rawPtr->metaObject()->className();
    } 
    else 
    {
        typeName = typeid(T).name();
    }

    QString creatorName = "no-parent";
    if constexpr (std::is_pointer_v<Creator> && std::is_base_of_v<QObject, std::remove_pointer_t<Creator>>)
    {
        if (creator != nullptr)
        {
            creatorName = creator->metaObject()->className();

            QVariant prop = creator->property("instance");
            if (prop.isValid() && prop.canConvert<QString>())
            {
                subComponent = prop.toString();
            }
        }
    }

    QString const addr = QString("0x%1").arg(reinterpret_cast<quintptr>(rawPtr), QT_POINTER_SIZE * 2, 16, QChar('0'));

    if (creator != nullptr)
    {
        qCDebug(category).noquote() << QString("|%1| Creating object of type '%2' by '%3' - [%4]")
            .arg(subComponent, typeName, creatorName, addr);
    }
    else
    {
        qCDebug(category).noquote() << QString("|%1| Creating object of type '%2' by '%3' - [%4]")
            .arg(subComponent, typeName, creatorName, addr);
    }

    auto deleter = [subComponent, typeName, &category](T* ptr) {
        objectDeleter<T>(ptr, subComponent, typeName, category);
    };

    return QSharedPointer<T>(rawPtr, deleter);
}

// Convenience overload without explicit category: defaults to memory_objects_track()
template<typename T, typename Creator, typename... Args>
QSharedPointer<T> makeTrackedShared(Creator creator, Args&&... args)
{
    return makeTrackedShared<T>(creator, hyperion_objects_track(), std::forward<Args>(args)...);
}

// Factory: create a tracked singleton (QObject-friendly) and assign storage
// Returns true if a new instance was created, false if already present
template<typename T>
bool createTrackedSingleton(QSharedPointer<T>& storage, QObject* parent)
{
    if (!storage) {
        storage = makeTrackedShared<T>(parent, parent);
        return !storage.isNull();
    }
    return false;
}

// Factory: create a tracked singleton (QObject-friendly) without assigning
template<typename T>
QSharedPointer<T> makeTrackedSingleton(QObject* parent)
{
    return makeTrackedShared<T>(parent, parent);
}

#endif // MEMORYTRACKER_H
