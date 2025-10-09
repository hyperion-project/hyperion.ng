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
#define USE_TRACKED_DELETE_LATER ENABLE_MEMORY_TRACKING

#if USE_TRACKED_SHARED_PTR
	#define MAKE_TRACKED_SHARED(T, ...) makeTrackedShared<T>(__VA_ARGS__)
#else
	#define MAKE_TRACKED_SHARED(T, ...) QSharedPointer<T>(new T(__VA_ARGS__), &QObject::deleteLater)
#endif

#if USE_TRACKED_DELETE_LATER
	#define DELETE_LATER_FN(T) trackedDeleteLater<T>
#else
	#define DELETE_LATER_FN(T) &QObject::deleteLater
#endif

// Deleter function template
template<typename T>
void trackedDeleteLater(T* ptr)
{
	if (!ptr)
		return;

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

	if constexpr (std::is_base_of<QObject, T>::value)
	{
		QThread* thread = ptr->thread();
		if (thread && thread->isRunning())
		{
			ptr->deleteLater();
			Debug(log, "QObject<%s>::deleteLater() scheduled on thread '%s'", QSTRING_CSTR(typeName), QSTRING_CSTR(thread->objectName()));
		}
		else
		{
			delete ptr;
			Debug(log, "QObject<%s> deleted immediately (thread not running).", QSTRING_CSTR(typeName));
		}
	}
	else
	{
		delete ptr;
		Debug(log, "Non-QObject<%s> deleted immediately.", QSTRING_CSTR(typeName));
	}
}

// Factory function template to create tracked QSharedPointer
template<typename T, typename... Args>
QSharedPointer<T> makeTrackedShared(Args&&... args)
{
	T* rawPtr = new T(std::forward<Args>(args)...);

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

	return QSharedPointer<T>(rawPtr, DELETE_LATER_FN(T));
}

#endif // TRACKEDMEMORY_H
