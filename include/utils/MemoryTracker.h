#ifndef MEMORYTRACKER_H
#define MEMORYTRACKER_H

#include <typeinfo>
#include <type_traits>

#include <QSharedPointer>
#include <QObject>
#include <QThread>
#include <QString>
#include <QDebug>
#include <QLoggingCategory>

#include <utils/global_defines.h>
#include <utils/Logger.h>

Q_DECLARE_LOGGING_CATEGORY(memory_objects_create)
Q_DECLARE_LOGGING_CATEGORY(memory_objects_track)
Q_DECLARE_LOGGING_CATEGORY(memory_objects_destroy)
Q_DECLARE_LOGGING_CATEGORY(memory_non_objects_create)
Q_DECLARE_LOGGING_CATEGORY(memory_non_objects_destroy)

#define MAKE_TRACKED_SHARED(T, ...) makeTrackedShared<T>(this, __VA_ARGS__)
#define MAKE_TRACKED_SHARED_STATIC(T, ...) makeTrackedShared<T>(nullptr, __VA_ARGS__)
#define TRACK_SCOPE qCDebug(memory_objects_track).noquote() << QString("%1()...").arg( __func__)
#define TRACK_SCOPE_SUBCOMPONENT qCDebug(memory_objects_track).noquote() << QString("|%1| %2()...").arg(_log->getSubName(), __func__)

template<typename T>
void objectDeleter(T* ptr, const QString& subComponent, const QString& typeName)
{
    if (!ptr)
    {
        return;
    }

    if constexpr (std::is_base_of_v<QObject, T>)
    {
        QThread const* thread = ptr->thread();
        if (thread && thread == QThread::currentThread()) {
            qCDebug(memory_objects_destroy).noquote() << QString("|%1| QObject<%2> scheduled for deletion.").arg(subComponent, typeName);
            ptr->deleteLater();
        }
        else
        {
            if (thread && thread->isRunning())
            {
                // Schedule deleteLater from the object's thread
                qCDebug(memory_objects_destroy).noquote() << QString("|%1| QObject<%2>::deleteLater() scheduled via invokeMethod on thread '%3'").arg(subComponent, typeName).arg(thread->objectName());
                QMetaObject::invokeMethod(ptr, "deleteLater", Qt::QueuedConnection);
            }
            else
            {
				// The object's thread is not running, which is a potential issue.
				// Log a critical error as this indicates a problem in the shutdown sequence.
                // This should be an *extremely rare* fallback and indicates a bug in the thread shutdown sequence.
				qCritical().noquote() << QString("|%1| QObject<%2> could not be deleted. Owning thread is not running.").arg(subComponent, typeName);

                //qCDebug(memory_objects).noquote() << QString("|%1| QObject<%2> object's owning thread is not running. Deleted immediately (thread not running)").arg(subComponent, typeName);
				//direct delete will require friendship, to be added (to Logger)
                //delete ptr;
            }
        }
    }
    else
    {
        qCDebug(memory_objects_destroy).noquote() << QString("|%1| Non-QObject<%2> deleted immediately.").arg(subComponent, typeName);
        delete ptr;
    }
}

// Factory function template to create tracked QSharedPointer
template<typename T, typename Creator, typename... Args>
QSharedPointer<T> makeTrackedShared(Creator creator, Args&&... args)
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

    QString creatorName = "non-QObject";
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

    if (creator != nullptr)
    {
        qCDebug(memory_objects_create).noquote() << QString("|%1| Creating object of type '%2' at %3 by '%4'")
            .arg(subComponent, typeName, QString("0x%1").arg(reinterpret_cast<quintptr>(rawPtr), QT_POINTER_SIZE * 2, 16, QChar('0')), creatorName);
    }
    else
    {
        qCDebug(memory_non_objects_create).noquote() << QString("|%1| Creating object of type '%2' at %3 by '%4'")
            .arg(subComponent, typeName, QString("0x%1").arg(reinterpret_cast<quintptr>(rawPtr), QT_POINTER_SIZE * 2, 16, QChar('0')), creatorName);
    }

    auto deleter = [subComponent, typeName](T* ptr) {
        objectDeleter<T>(ptr, subComponent, typeName);
    };

    return QSharedPointer<T>(rawPtr, deleter);
}

#endif // MEMORYTRACKER_H
