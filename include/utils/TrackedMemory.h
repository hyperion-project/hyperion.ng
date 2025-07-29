#ifndef TRACKEDMEMORY_H
#define TRACKEDMEMORY_H

#include <QSharedPointer>
#include <QObject>
#include <QThread>
#include <QString>
#include <typeinfo>
#include <type_traits>
#include <utils/Logger.h>

#define ENABLE_MEMORY_TRACKING 0

#define USE_TRACKED_SHARED_PTR ENABLE_MEMORY_TRACKING
#define USE_TRACKED_CUSTOM_DELETE ENABLE_MEMORY_TRACKING

#if USE_TRACKED_SHARED_PTR
	#define MAKE_TRACKED_SHARED(T, ...) makeTrackedShared<T>(__VA_ARGS__)
#else
	#define MAKE_TRACKED_SHARED(T, ...) QSharedPointer<T>(new T(__VA_ARGS__), &customDelete<T>)
#endif

// Custom Delete function templates

template<typename T>
void customDelete(T* ptr)
{
	if (!ptr)
		return;

#if USE_TRACKED_CUSTOM_DELETE

	QString subComponent = "__";
	QString typeName;

	if constexpr (std::is_base_of<QObject, T>::value)
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

	Logger* log = Logger::getInstance("MEMORY", subComponent);
	Debug(log, "Deleting object of type '%s' at %p", QSTRING_CSTR(typeName), static_cast<void*>(ptr));
#endif

	if constexpr (std::is_base_of<QObject, T>::value)
	{
		QThread* thread = ptr->thread();
		if (thread && thread == QThread::currentThread()) {
#if USE_TRACKED_CUSTOM_DELETE
			Debug(log, "QObject<%s> deleted immediately (current thread).", QSTRING_CSTR(typeName));
#endif
			ptr->deleteLater();
		}
		else
		{
			if (thread && thread->isRunning())
			{
				// Schedule deleteLater from the object's thread
#if USE_TRACKED_CUSTOM_DELETE
				Debug(log, "QObject<%s>::deleteLater() scheduled via invokeMethod on thread '%s'",
					QSTRING_CSTR(typeName), QSTRING_CSTR(thread->objectName()));
#endif
				QMetaObject::invokeMethod(ptr, "deleteLater", Qt::QueuedConnection);
			}
			else
			{
				// This should be an *extremely rare* fallback and indicates a bug in the thread shutdown sequence.
#if USE_TRACKED_CUSTOM_DELETE
				Debug(log, "<%s> object's owning thread is not running. Deleted immediately (thread not running).", QSTRING_CSTR(typeName));
#endif
				delete ptr;
			}
		}
	}
	else
	{
#if USE_TRACKED_CUSTOM_DELETE
		Debug(log, "Non-QObject<%s> deleted immediately.", QSTRING_CSTR(typeName));
#endif
		delete ptr;

	}
}

// Factory function template to create tracked QSharedPointer
template<typename T, typename... Args>
QSharedPointer<T> makeTrackedShared(Args&&... args)
{
	T* rawPtr = new T(std::forward<Args>(args)...);
	if (!rawPtr)
		return QSharedPointer<T>();

	QString subComponent = "__";
	QString typeName;

	if constexpr (std::is_base_of<QObject, T>::value)
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

	Logger* log = Logger::getInstance("MEMORY", subComponent);
	Debug(log, "Creating object of type '%s' at %p", QSTRING_CSTR(typeName), static_cast<void*>(rawPtr));

	return QSharedPointer<T>(rawPtr, &customDelete<T>);
}

#endif // TRACKEDMEMORY_H
