#ifndef TRACKEDMEMORY_H
#define TRACKEDMEMORY_H

#include <QSharedPointer>
#include <QObject>
#include <QThread>
#include <QString>
#include <typeinfo>
#include <type_traits>
#include <utils/Logger.h>

/*
 * EXAMPLE USAGE:
 *
 * 0. To enable tracing globally for all classes, define the following macro
 *    before including TrackedMemory.h (e.g., in your CMake file):
 *
 *    #define ENABLE_GLOBAL_MEMORY_TRACKING
 *
 * 1. To enable tracing for a class at runtime (e.g., in your main.cpp):
 *
 *    #include <utils/Image.h>
 *    #include <utils/ColorRgb.h>
 *    #include <utils/TrackedMemory.h>
 *
 *    Enable all trace events for Image<ColorRgb>
 *    ComponentTracer<Image<ColorRgb>>::active_events = TraceEvent::All;
 *
 *    Or just specific events
 *    ComponentTracer<Image<ColorRgb>>::active_events = TraceEvent::Alloc | TraceEvent::Deep;
 *
 *    Enable tracing for the underlying data of Image<ColorRgb>
 *    ComponentTracer<ImageData<ColorRgb>>::active_events = TraceEvent::All;
 *
 * 2. To disable all tracing for a class at compile time, you can specialize the template:
 *
 *    template<>
 *    struct ComponentTracer<Image<ColorRgb>> {
 *        static constexpr bool enabled = false;
 *        static inline TraceEvent active_events = TraceEvent::None;
 *    };
 *
 */

// Generic flags for memory/lifetime event tracing
enum class TraceEvent : std::uint32_t {
	None    = 0,
	Alloc   = 1 << 0, // For new resource allocations
	Deep    = 1 << 1, // For deep copies (detach)
	Shallow = 1 << 2, // For shallow copies (ref-counting)
	Move    = 1 << 3, // For move operations
	Release = 1 << 4, // For resource/handle destruction
	All     = Alloc | Deep | Shallow | Move | Release
};

// Enable bitwise operators for the enum class
constexpr TraceEvent operator|(TraceEvent a, TraceEvent b) {
	return static_cast<TraceEvent>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr TraceEvent operator&(TraceEvent a, TraceEvent b) {
	return static_cast<TraceEvent>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr bool has_flag(TraceEvent haystack, TraceEvent needle) {
	return (static_cast<std::uint32_t>(haystack) & static_cast<std::uint32_t>(needle)) != 0;
}

// A generic, templated component tracer.
// This allows controlling trace events on a per-class basis.
template<typename T>
struct ComponentTracer {
	// Statically holds the active trace events for class T.
	// Can be modified at runtime to dynamically change logging verbosity.
	// Default is None, so no tracing occurs unless explicitly enabled.
#if defined(ENABLE_GLOBAL_MEMORY_TRACKING)
	static inline TraceEvent active_events = TraceEvent::All;
	static inline bool enabled = true;
#else
	static inline TraceEvent active_events = TraceEvent::None;
	static inline bool enabled = false;
#endif
};

// Helper function to check if a specific event is being traced for a class.
template<typename T>
inline bool is_tracing(TraceEvent event) {
	if (ComponentTracer<T>::enabled)
	{
		return has_flag(ComponentTracer<T>::active_events, event);
	}
	return false;
}

// Forward declarations
template<typename T>
void customDelete(T* ptr);

template<typename T, typename... Args>
QSharedPointer<T> makeTrackedShared(Args&&... args);

// Main factory helper function
template<typename T, typename... Args>
QSharedPointer<T> createTrackedPointer(Args&&... args)
{
	if (ComponentTracer<T>::enabled)
	{
		return makeTrackedShared<T>(std::forward<Args>(args)...);
	}
	else
	{
		return QSharedPointer<T>(new T(std::forward<Args>(args)...), &customDelete<T>);
	}
}

#define MAKE_TRACKED_SHARED(T, ...) createTrackedPointer<T>(__VA_ARGS__)

// Custom Delete function templates
template<typename T>
void customDelete(T* ptr)
{
	if (!ptr)
		return;

	if (is_tracing<T>(TraceEvent::Release))
	{
		QString subComponent = "__";
		QString typeName;

		if constexpr (std::is_base_of_v<QObject, T>)
		{
			if (QVariant prop = ptr->property("instance"); prop.isValid() && prop.canConvert<QString>())
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
		Debug(log, "Deleting object of type '%s' at %p - current thread: '%s'", QSTRING_CSTR(typeName), static_cast<void*>(ptr), QSTRING_CSTR(QThread::currentThread()->objectName()));
	}

	if constexpr (std::is_base_of_v<QObject, T>)
	{
		ptr->deleteLater();
	}
	else
	{
		delete ptr;
	}
}

// Factory function template to create tracked QSharedPointer
template<typename T, typename... Args>
QSharedPointer<T> makeTrackedShared(Args&&... args)
{
	auto rawPtr = new T(std::forward<Args>(args)...);
	if (!rawPtr)
		return QSharedPointer<T>();

	if (is_tracing<T>(TraceEvent::Alloc))
	{
		QString subComponent = "__";
		QString typeName;

		if constexpr (std::is_base_of_v<QObject, T>)
		{
			if (QVariant prop = rawPtr->property("instance"); prop.isValid() && prop.canConvert<QString>())
			{
				subComponent = prop.toString();
			}
			typeName = rawPtr->metaObject()->className();
		}
		{
			typeName = typeid(T).name();
		}

		Logger* log = Logger::getInstance("MEMORY", subComponent);
		Debug(log, "Creating object of type '%s' at %p (current thread '%s')", QSTRING_CSTR(typeName), static_cast<void*>(rawPtr), QSTRING_CSTR(QThread::currentThread()->objectName()));
	}

	return QSharedPointer<T>(rawPtr, &customDelete<T>);
}

#endif // TRACKEDMEMORY_H
