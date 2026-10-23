#ifndef OSEVENTHANDLER_H
#define OSEVENTHANDLER_H

#include <QObject>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QScopedPointer>

#include <events/EventEnum.h>

#if defined(_WIN32)
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>
#include <QWidget>
#include <windows.h>
#include <powrprof.h>
#endif

#include <utils/Logger.h>
#include <utils/settings.h>

Q_DECLARE_LOGGING_CATEGORY(event_os);

class Logger;

// ---------------------------------------------------------------------------
// Singleton mixin — provides getInstance() / destroyInstance() for the exact
// concrete type Derived without touching QObject or requiring Q_OBJECT.
// ---------------------------------------------------------------------------
template <typename Derived>
class OsEventHandlerSingleton
{
public:
	static QScopedPointer<Derived>& getInstance()
	{
		if (instance.isNull())
			instance.reset(new Derived());
		return instance;
	}

	static void destroyInstance()
	{
		instance.reset();
	}

protected:
	static QScopedPointer<Derived> instance;
};

template <typename Derived>
QScopedPointer<Derived> OsEventHandlerSingleton<Derived>::instance;

// ---------------------------------------------------------------------------
// Base class — shared logic, NO singleton machinery here.
// ---------------------------------------------------------------------------
class OsEventHandlerBase : public QObject
{
	Q_OBJECT

public:
	~OsEventHandlerBase() override;

public slots:
	void suspend(bool sleep);
	void lock(bool isLocked);
	void quit();

	virtual void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

signals:
	void signalEvent(Event event);

protected:
	virtual bool registerOsEventHandler() { return true; }
	virtual void unregisterOsEventHandler() {}
	virtual bool registerLockHandler() { return true; }
	virtual void unregisterLockHandler() {}

public:
	virtual bool isLockHandlerRegistered() { return _isLockRegistered; }
	virtual bool isSuspendHandlerRegistered() { return _isSuspendRegistered; }

protected:
	bool _isSuspendEnabled;
	bool _isLockEnabled;
	bool _isSuspendOnLock;

	bool _isSuspendRegistered;
	bool _isLockRegistered;

	QSharedPointer<Logger> _log;

	OsEventHandlerBase();
	OsEventHandlerBase(const OsEventHandlerBase&) = delete;
	OsEventHandlerBase& operator=(const OsEventHandlerBase&) = delete;

private:
	void applyLockState();
};

// ---------------------------------------------------------------------------
// Platform-specific subclasses — each inherits both the base and the mixin.
// ---------------------------------------------------------------------------

#if defined(_WIN32)

class OsEventHandlerWindows : public OsEventHandlerBase,
                              public OsEventHandlerSingleton<OsEventHandlerWindows>,
                              public QAbstractNativeEventFilter
{
	friend class OsEventHandlerSingleton<OsEventHandlerWindows>;

public:
	~OsEventHandlerWindows();

	void handleSuspendResumeEvent(bool sleep);

protected:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
#else
	bool nativeEventFilter(const QByteArray& eventType, void* message, long int* result) override;
#endif

	bool registerOsEventHandler() override;
	void unregisterOsEventHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;

private:
	OsEventHandlerWindows();

	static DEVICE_NOTIFY_CALLBACK_ROUTINE handlePowerNotifications;

	QWidget* _widget;
	HPOWERNOTIFY _notifyHandle;
};

using OsEventHandler = OsEventHandlerWindows;

#elif defined(__linux__)

class OsEventHandlerLinux : public OsEventHandlerBase,
                            public OsEventHandlerSingleton<OsEventHandlerLinux>
{
	Q_OBJECT
	friend class OsEventHandlerSingleton<OsEventHandlerLinux>;

	static void static_signaleHandler(int signum)
	{
		OsEventHandlerLinux::getInstance()->handleSignal(signum);
	}

public:
	void handleSignal(int signum);

private:
	OsEventHandlerLinux();

#if defined(HYPERION_HAS_DBUS)
	bool registerOsEventHandler() override;
	void unregisterOsEventHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;
#endif
};

using OsEventHandler = OsEventHandlerLinux;

#elif defined(__APPLE__)

class OsEventHandlerMacOS : public OsEventHandlerBase,
                            public OsEventHandlerSingleton<OsEventHandlerMacOS>
{
	Q_OBJECT
	friend class OsEventHandlerSingleton<OsEventHandlerMacOS>;

private:
	OsEventHandlerMacOS();

	bool registerOsEventHandler() override;
	void unregisterOsEventHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;

	void* _sleepEventHandler;
	void* _lockEventHandler;
};

using OsEventHandler = OsEventHandlerMacOS;

#else

// Fallback: base itself is the concrete singleton target.
class OsEventHandlerFallback : public OsEventHandlerBase,
                               public OsEventHandlerSingleton<OsEventHandlerFallback>
{
	friend class OsEventHandlerSingleton<OsEventHandlerFallback>;
	OsEventHandlerFallback() = default;
};

using OsEventHandler = OsEventHandlerFallback;

#endif

#endif // OSEVENTHANDLER_H
