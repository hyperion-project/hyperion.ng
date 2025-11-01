#include "events/OsEventHandler.h"

#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QApplication>

#include <events/EventHandler.h>
#include <utils/Logger.h>

#if defined(_WIN32)

#include <QWidget>
#include <windows.h>
#include <wtsapi32.h>
#include <powerbase.h>

#pragma comment( lib, "wtsapi32.lib" )
#pragma comment(lib, "PowrProf.lib")

#elif defined(__APPLE__)
#include <AppKit/AppKit.h>
#endif

OsEventHandlerBase::OsEventHandlerBase()
	: _isSuspendEnabled(false)
	, _isLockEnabled(false)
	, _isSuspendOnLock(false)
	, _isSuspendRegistered(false)
	, _isLockRegistered(false)
	, _isService(false)
{
	TRACK_SCOPE;
	qRegisterMetaType<Event>("Event");
	_log = Logger::getInstance("EVENTS-OS");

	QCoreApplication* app = QCoreApplication::instance();
	if (qobject_cast<QApplication*>(app) == nullptr)
	{
		_isService = true;
	}
	QObject::connect(this, &OsEventHandlerBase::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);

	Debug(_log, "Operating System event handler created");
}

OsEventHandlerBase::~OsEventHandlerBase()
{
	TRACK_SCOPE;
	quit();
	QObject::disconnect(this, &OsEventHandlerBase::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);

	OsEventHandlerBase::unregisterLockHandler();
	OsEventHandlerBase::unregisterOsEventHandler();

	Info(_log, "Operating System event handler stopped");
}

void OsEventHandlerBase::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::OSEVENTS)
	{
		const QJsonObject& obj = config.object();

		//Suspend on lock or go into idle mode
		bool prevIsSuspendOnLock = _isSuspendOnLock;
		_isSuspendOnLock = obj["suspendOnLockEnable"].toBool(false);

		//Handle OS event related configurations
		_isSuspendEnabled = obj["suspendEnable"].toBool(true);
		if (_isSuspendEnabled)
		{
			// Listen to suspend/resume/idle events received by the OS
			registerOsEventHandler();
		}
		else
		{
			unregisterOsEventHandler();
		}

		if (!_isService)
		{
			_isLockEnabled = obj["lockEnable"].toBool(true);
			if (_isLockEnabled || _isSuspendOnLock != prevIsSuspendOnLock)
			{
				// Listen to lock/screensaver events received by the OS
				registerLockHandler();
			}
			else
			{
				unregisterLockHandler();
			}
		}
	}
}

void OsEventHandlerBase::suspend(bool sleep)
{
	if (sleep)
	{
		emit signalEvent(Event::Suspend);
	}
	else
	{
		emit signalEvent(Event::Resume);
	}
}

void OsEventHandlerBase::lock(bool isLocked)
{
	if (isLocked)
	{
		if (_isSuspendOnLock)
		{
			emit signalEvent(Event::Suspend);
		}
		else
		{
			emit signalEvent(Event::Idle);
		}
	}
	else
	{
		if (_isSuspendOnLock)
		{
			emit signalEvent(Event::Resume);
		}
		else
		{
			emit signalEvent(Event::ResumeIdle);
		}
	}
}

void OsEventHandlerBase::quit()
{
	emit signalEvent(Event::Quit);
}

#if defined(_WIN32)

OsEventHandlerWindows* OsEventHandlerWindows::getInstance()
{
	static OsEventHandlerWindows instance;
	return &instance;
}

OsEventHandlerWindows::OsEventHandlerWindows()
	: _notifyHandle(NULL),
	_widget(nullptr)
{
}

OsEventHandlerWindows::~OsEventHandlerWindows()
{
	unregisterLockHandler();
	unregisterOsEventHandler();

	delete _widget;
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool OsEventHandlerWindows::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* /*result*/)
#else
bool OsEventHandlerWindows::nativeEventFilter(const QByteArray& eventType, void* message, long int* /*result*/)
#endif
{

	MSG* msg = static_cast<MSG*>(message);
	switch (msg->message)
	{
	case WM_WTSSESSION_CHANGE:
		switch (msg->wParam)
		{
		case WTS_SESSION_LOCK:
			emit lock(true);
			return true;
			break;
		case WTS_SESSION_UNLOCK:
			emit lock(false);
			return true;
			break;
		}
		break;
	}
	return false;
}

void OsEventHandlerWindows::handleSuspendResumeEvent(bool sleep)
{
	if (sleep)
	{
		suspend(true);
	}
	else
	{
		suspend(false);
	}
}

ULONG OsEventHandlerWindows::handlePowerNotifications(PVOID Context, ULONG Type, PVOID Setting)
{
	switch (Type)
	{
	case PBT_APMRESUMESUSPEND:
		getInstance()->handleSuspendResumeEvent(false);
		break;
	case PBT_APMSUSPEND:
		getInstance()->handleSuspendResumeEvent(true);
		break;
	}
	return S_OK;
}

bool OsEventHandlerWindows::registerOsEventHandler()
{
	bool isRegistered{ _isSuspendRegistered };
	if (!_isSuspendRegistered)
	{
		DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS notificationsParameters = { handlePowerNotifications, NULL };
		if (PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, (HANDLE)&notificationsParameters, &_notifyHandle) == ERROR_SUCCESS)
		{
			isRegistered = true;
		}
		else
		{
			Error(_log, "Could not register for suspend/resume events!");
		}

		if (isRegistered)
		{
			_isSuspendRegistered = true;
		}
	}
	return isRegistered;
}

void OsEventHandlerWindows::unregisterOsEventHandler()
{
	if (_isSuspendRegistered)
	{
		if (_notifyHandle != NULL)
		{
			PowerUnregisterSuspendResumeNotification(_notifyHandle);
		}
		_notifyHandle = NULL;
		_isSuspendRegistered = false;
	}
}

bool OsEventHandlerWindows::registerLockHandler()
{
	bool isRegistered{ _isLockRegistered };
	if (!_isLockRegistered)
	{
		if (!_isService)
		{
			_widget = new QWidget();
			if (_widget)
			{
				auto handle = reinterpret_cast<HWND> (_widget->winId());
				if (WTSRegisterSessionNotification(handle, NOTIFY_FOR_THIS_SESSION))
				{
					QCoreApplication::instance()->installNativeEventFilter(this);
					isRegistered = true;
				}
				else
				{
					Error(_log, "Could not register for lock/unlock events!");
				}
			}
		}
	}

	if (isRegistered)
	{
		_isLockRegistered = true;
	}
	return isRegistered;
}

void OsEventHandlerWindows::unregisterLockHandler()
{
	if (_isLockRegistered)
	{
		if (!_isService)
		{
			auto handle = reinterpret_cast<HWND> (_widget->winId());
			QCoreApplication::instance()->removeNativeEventFilter(this);
			WTSUnRegisterSessionNotification(handle);
			_isLockRegistered = false;
		}
	}
}

#elif defined(__linux__)

#include <csignal>

OsEventHandlerLinux* OsEventHandlerLinux::getInstance()
{
	static OsEventHandlerLinux instance;
	return &instance;
}

OsEventHandlerLinux::OsEventHandlerLinux()
{
	signal(SIGUSR1, static_signaleHandler);
	signal(SIGUSR2, static_signaleHandler);
}

void OsEventHandlerLinux::handleSignal(int signum)
{
	if (signum == SIGUSR1)
	{
		suspend(true);
	}
	else if (signum == SIGUSR2)
	{
		suspend(false);
	}
}

#if defined(HYPERION_HAS_DBUS)
#include <QDBusConnection>

struct dBusSignals
{
	QString service;
	QString path;
	QString interface;
	QString name;
};

typedef QMultiMap<QString, dBusSignals> DbusSignalsMap;

// Constants
namespace {
	const DbusSignalsMap dbusSignals = {
		//system signals
		{"Suspend", {"org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager","PrepareForSleep"}},

		//Session signals
		{"ScreenSaver", {"org.freedesktop.ScreenSaver","/org/freedesktop/ScreenSaver","org.freedesktop.ScreenSaver","ActiveChanged"}},
		{"ScreenSaver", {"org.gnome.ScreenSaver","/org/gnome/ScreenSaver","org.gnome.ScreenSaver","ActiveChanged"}},
	};
} //End of constants

bool OsEventHandlerLinux::registerOsEventHandler()
{

	bool isRegistered{ _isSuspendRegistered };
	if (!_isSuspendRegistered)
	{
		QDBusConnection systemBus = QDBusConnection::systemBus();
		if (!systemBus.isConnected())
		{
			Info(_log, "The suspend/resume feature is not supported by your system configuration");
		}
		else
		{
			QString service = dbusSignals.find("Suspend").value().service;
			if (systemBus.connect(service,
				dbusSignals.find("Suspend").value().path,
				dbusSignals.find("Suspend").value().interface,
				dbusSignals.find("Suspend").value().name,
				this, SLOT(suspend(bool))))
			{
				Debug(_log, "Registered for suspend/resume events via service: %s", QSTRING_CSTR(service));
				isRegistered = true;
			}
			else
			{
				Error(_log, "Could not register for suspend/resume events via service: %s", QSTRING_CSTR(service));
			}
		}

		if (isRegistered)
		{
			_isSuspendRegistered = true;
		}

	}
	return isRegistered;
}

void OsEventHandlerLinux::unregisterOsEventHandler()
{
	if (_isSuspendRegistered)
	{
		QDBusConnection systemBus = QDBusConnection::systemBus();
		if (!systemBus.isConnected())
		{
			Info(_log, "The suspend/resume feature is not supported by your system configuration");
		}
		else
		{
			QString service = dbusSignals.find("Suspend").value().service;
			if (systemBus.disconnect(service,
				dbusSignals.find("Suspend").value().path,
				dbusSignals.find("Suspend").value().interface,
				dbusSignals.find("Suspend").value().name,
				this, SLOT(suspend(bool))))
			{
				Debug(_log, "Unregistered for suspend/resume events via service: %s", QSTRING_CSTR(service));
				_isSuspendRegistered = false;
			}
			else
			{
				Error(_log, "Could not unregister for suspend/resume events via service: %s", QSTRING_CSTR(service));
			}
		}
	}
}

bool OsEventHandlerLinux::registerLockHandler()
{
	bool isRegistered{ _isLockRegistered };

	if (!_isLockRegistered)
	{
		QDBusConnection sessionBus = QDBusConnection::sessionBus();
		if (!sessionBus.isConnected())
		{
			Info(_log, "The lock/unlock feature is not supported by your system configuration");
		}
		else
		{
			DbusSignalsMap::const_iterator iter = dbusSignals.find("ScreenSaver");
			while (iter != dbusSignals.end() && iter.key() == "ScreenSaver") {
				QString service = iter.value().service;
				if (sessionBus.connect(service,
					iter.value().path,
					iter.value().interface,
					iter.value().name,
					this, SLOT(lock(bool))))
				{
					Debug(_log, "Registered for lock/unlock events via service: %s", QSTRING_CSTR(service));
					isRegistered = true;
				}
				else
				{
					Error(_log, "Could not register for lock/unlock events via service: %s", QSTRING_CSTR(service));

				}
				++iter;
			}

		}
	}

	if (isRegistered)
	{
		_isLockRegistered = true;
	}

	return isRegistered;
}

void OsEventHandlerLinux::unregisterLockHandler()
{
	bool isUnregistered{ false };

	if (_isLockRegistered)
	{
		QDBusConnection sessionBus = QDBusConnection::sessionBus();
		if (!sessionBus.isConnected())
		{
			Info(_log, "The lock/unlock feature is not supported by your system configuration");
		}
		else
		{
			DbusSignalsMap::const_iterator iter = dbusSignals.find("ScreenSaver");
			while (iter != dbusSignals.end() && iter.key() == "ScreenSaver") {
				QString service = iter.value().service;
				if (sessionBus.disconnect(service,
					iter.value().path,
					iter.value().interface,
					iter.value().name,
					this, SLOT(lock(bool))))
				{
					Debug(_log, "Unregistered for lock/unlock events via service: %s", QSTRING_CSTR(service));
					isUnregistered = true;
				}
				else
				{
					Error(_log, "Could not unregister for lock/unlock events via service: %s", QSTRING_CSTR(service));

				}
				++iter;
			}

			if (isUnregistered)
			{
				_isLockRegistered = false;
			}
		}
	}
}
#endif // HYPERION_HAS_DBUS

#elif defined(__APPLE__)

OsEventHandlerMacOS::OsEventHandlerMacOS()
	: _sleepEventHandler(nullptr)
	, _lockEventHandler(nullptr)
{
}

@interface SleepEvents : NSObject
{
	OsEventHandlerMacOS* _eventHandler;
}
- (id)initSleepEvents : (OsEventHandlerMacOS*)osEventHandler;
@end

@implementation SleepEvents
- (id)initSleepEvents:(OsEventHandlerMacOS*)osEventHandler
{
	if ((self = [super init]))
	{
		_eventHandler = osEventHandler;
		id notifCenter = [[NSWorkspace sharedWorkspace]notificationCenter];
		[notifCenter addObserver : self selector : @selector(receiveSleepWake:) name:NSWorkspaceWillSleepNotification object : nil] ;
		[notifCenter addObserver : self selector : @selector(receiveSleepWake:) name:NSWorkspaceDidWakeNotification object : nil] ;
	}

	return self;
}

- (void)dealloc
{
	[[[NSWorkspace sharedWorkspace]notificationCenter] removeObserver:self];
	_eventHandler = nullptr;
	[super dealloc] ;
}

- (void)receiveSleepWake:(NSNotification*)notification
{
	if (!_eventHandler) return;
	if (notification.name == NSWorkspaceWillSleepNotification)
	{
		_eventHandler->suspend(true);
	}
	else if (notification.name == NSWorkspaceDidWakeNotification)
	{
		_eventHandler->suspend(false);
	}
}
@end

bool OsEventHandlerMacOS::registerOsEventHandler()
{
	bool isRegistered{ _isSuspendRegistered };
	if (!_isSuspendRegistered)
	{
		_sleepEventHandler = [[SleepEvents alloc]initSleepEvents:this];
		if (_sleepEventHandler)
		{
			Debug(_log, "Registered for suspend/resume events");
			isRegistered = true;
		}
		else
		{
			Error(_log, "Could not register for suspend/resume events");
		}

		if (isRegistered)
		{
			_isSuspendRegistered = true;
		}
	}
	return isRegistered;
}

void OsEventHandlerMacOS::unregisterOsEventHandler()
{
	if (_isSuspendRegistered && _sleepEventHandler)
	{
		[(SleepEvents*)_sleepEventHandler release] , _sleepEventHandler = nil;
		if (!_sleepEventHandler)
		{
			Debug(_log, "Unregistered for suspend/resume events");
			_isSuspendRegistered = false;
		}
		else
		{
			Error(_log, "Could not unregister for suspend/resume events");
		}
	}
}

@interface LockEvents : NSObject
{
	OsEventHandlerMacOS* _eventHandler;
}
- (id)initLockEvents : (OsEventHandlerMacOS*)osEventHandler;
@end

@implementation LockEvents
- (id)initLockEvents:(OsEventHandlerMacOS*)osEventHandler
{
	if ((self = [super init]))
	{
		_eventHandler = osEventHandler;
		id defCenter = [NSDistributedNotificationCenter defaultCenter];
		[defCenter addObserver : self selector : @selector(receiveLockUnlock:) name:@"com.apple.screenIsLocked" object:nil] ;
		[defCenter addObserver : self selector : @selector(receiveLockUnlock:) name:@"com.apple.screenIsUnlocked" object:nil] ;
	}

	return self;
}

- (void)dealloc
{
	[[[NSWorkspace sharedWorkspace]notificationCenter] removeObserver:self];
	_eventHandler = nullptr;
	[super dealloc] ;
}

- (void)receiveLockUnlock:(NSNotification*)notification
{
	if (!_eventHandler) return;
	if (CFEqual(notification.name, CFSTR("com.apple.screenIsLocked")))
	{
		_eventHandler->lock(true);
	}
	else if (CFEqual(notification.name, CFSTR("com.apple.screenIsUnlocked")))
	{
		_eventHandler->lock(false);
	}
}
@end

bool OsEventHandlerMacOS::registerLockHandler()
{
	bool isRegistered{ _isLockRegistered };
	if (!_isLockRegistered)
	{
		_lockEventHandler = [[LockEvents alloc]initLockEvents:this];
		if (_lockEventHandler)
		{
			Debug(_log, "Registered for lock/unlock events");
			isRegistered = true;
		}
		else
		{
			Error(_log, "Could not register for lock/unlock events!");
		}
	}

	if (isRegistered)
	{
		_isLockRegistered = true;
	}
	return isRegistered;
}

void OsEventHandlerMacOS::unregisterLockHandler()
{
	if (_isLockRegistered && _lockEventHandler)
	{
		[(LockEvents*)_lockEventHandler release] , _lockEventHandler = nil;
		if (!_lockEventHandler)
		{
			Debug(_log, "Unregistered for lock/unlock events");
			_isLockRegistered = false;
		}
		else
		{
			Error(_log, "Could not unregister for lock/unlock events");
		}
	}
}

#endif
