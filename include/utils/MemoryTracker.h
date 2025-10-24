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

Q_DECLARE_LOGGING_CATEGORY(memory_objects)

#define MAKE_TRACKED_SHARED(T, ...) makeTrackedShared<T>(__VA_ARGS__)

// Custom Delete function templates
template<typename T>
void customDelete(T* ptr)
{
	if (!ptr)
	{
		return;
	}

	QString subComponent = "__";
	QString typeName;	
	//Only resolve subComponent and typeName if debug is enabled
	if (memory_objects().isDebugEnabled())
	{
		if constexpr (std::is_base_of_v<QObject, T>)
		{
			QVariant prop = ptr->property("instance");
			if (prop.isValid() && prop.canConvert<QString>())
			{
				subComponent = prop.toString();
			}
			typeName = ptr->metaObject()->className();
		}
		else
		{
			typeName = typeid(T).name();
		}

		// qCDebug(memory_objects).noquote() << QString("Deleting object of type '%1' at %2")
		// 	.arg(typeName)
		// 	.arg(QString("0x%1").arg(reinterpret_cast<quintptr>(ptr), 16, 16, QChar('0'))));
	}

	if constexpr (std::is_base_of_v<QObject, T>)
	{
		QThread const*  thread = ptr->thread();
		if (thread && thread == QThread::currentThread()) {
            qCDebug(memory_objects).noquote() << QString("|%1| QObject<%2> deleted immediately.").arg(subComponent,typeName);
            ptr->deleteLater();
        }
        else
        {
            if (thread && thread->isRunning())
            {
                // Schedule deleteLater from the object's thread
                qCDebug(memory_objects).noquote() << QString("|%1| QObject<%2>::deleteLater() scheduled via invokeMethod on thread '%3'").arg(subComponent,typeName).arg(thread->objectName());
                QMetaObject::invokeMethod(ptr, "deleteLater", Qt::QueuedConnection);
            }
            else
            {
                // This should be an *extremely rare* fallback and indicates a bug in the thread shutdown sequence.
                qCDebug(memory_objects).noquote() << QString("|%1| QObject<%2> object's owning thread is not running. Deleted immediately (thread not running)").arg(subComponent,typeName);
                delete ptr;
            }
        }
	}
	else
	{
        qCDebug(memory_objects).noquote() << QString("|%1| Non-QObject<%2> deleted immediately.").arg(subComponent,typeName);
        delete ptr;
	}
}

// Factory function template to create tracked QSharedPointer
template<typename T, typename... Args>
QSharedPointer<T> makeTrackedShared(Args&&... args)
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
		QVariant prop = rawPtr->property("instance");
		if (prop.isValid() && prop.canConvert<QString>())
		{
			subComponent = prop.toString();
		}
		typeName = rawPtr->metaObject()->className();
	}
	else
	{
		typeName = typeid(T).name();
	}

    qCDebug(memory_objects).noquote() << QString("|%1| Creating object of type '%2' at %3")
        .arg(subComponent)
        .arg(typeName)
        .arg(QString("0x%1").arg(reinterpret_cast<quintptr>(rawPtr), 16, 16, QChar('0')));

    return QSharedPointer<T>(rawPtr, &customDelete<T>);
}

#endif // MEMORYTRACKER_H
