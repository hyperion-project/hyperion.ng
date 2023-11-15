#include "OsEventHandler.h"

#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonObject>

#include <events/EventHandler.h>
#include <utils/Logger.h>

#include <iostream>

#if defined(_WIN32)
#include <QCoreApplication>
#include <QWidget>
#include <windows.h>
#include <wtsapi32.h>

#pragma comment( lib, "wtsapi32.lib" )
#endif

OsEventHandlerBase::OsEventHandlerBase()
	: _isSuspendEnabled(false)
	, _isLockEnabled(false)
	, _isSuspendOnLock(false)
	, _isSuspendRegistered(false)
	, _isLockRegistered(false)
{
	qRegisterMetaType<Event>("Event");
	_log = Logger::getInstance("EVENTS");

	QObject::connect(this, &OsEventHandlerBase::signalEvent, EventHandler::getInstance(), &EventHandler::handleEvent);
}

OsEventHandlerBase::~OsEventHandlerBase()
{
	QObject::disconnect(this, &OsEventHandlerBase::signalEvent, EventHandler::getInstance(), &EventHandler::handleEvent);

	OsEventHandlerBase::unregisterLockHandler();
	OsEventHandlerBase::unregisterOsEventHandler();
}

void OsEventHandlerBase::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::OSEVENTS)
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

#if defined(_WIN32)

OsEventHandlerWindows::OsEventHandlerWindows()
	: _notifyHandle(NULL)
{
}

OsEventHandlerWindows::~OsEventHandlerWindows()
{
	unregisterLockHandler();
	unregisterOsEventHandler();
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

	case WM_POWERBROADCAST:
		switch (msg->wParam)
		{
		case PBT_APMRESUMESUSPEND:
			emit suspend(false);
			return true;
			break;
		case PBT_APMSUSPEND:
			emit suspend(true);
			return true;
			break;
		}
		break;
	}
	return false;
}

bool OsEventHandlerWindows::registerOsEventHandler()
{
	bool isRegistered{ _isSuspendRegistered };
	if (!_isSuspendRegistered)
	{
		auto handle = reinterpret_cast<HWND> (_widget.winId());
		_notifyHandle = RegisterSuspendResumeNotification(handle, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (_notifyHandle != NULL)
		{
			QCoreApplication::instance()->installNativeEventFilter(this);
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
			QCoreApplication::instance()->removeNativeEventFilter(this);
			UnregisterSuspendResumeNotification(_notifyHandle);
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
		auto handle = reinterpret_cast<HWND> (_widget.winId());
		if (WTSRegisterSessionNotification(handle, NOTIFY_FOR_THIS_SESSION))
		{
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

void OsEventHandlerWindows::unregisterLockHandler()
{
	if (_isLockRegistered)
	{
		auto handle = reinterpret_cast<HWND> (_widget.winId());
		WTSUnRegisterSessionNotification(handle);
		_isLockRegistered = false;
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

void OsEventHandlerLinux::handleSignal (int signum)
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

	bool isRegistered {_isSuspendRegistered};
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
	bool isRegistered {_isLockRegistered};

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
	bool isUnregistered {false};

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

OsEventHandlerMacOS* OsEventHandlerMacOS::getInstance()
{
	static OsEventHandlerMacOS instance;
	return &instance;
}

void OsEventHandlerMacOS::handleSignal (CFStringRef lock_unlock)
{
	if (CFEqual(lock_unlock, CFSTR("com.apple.screenIsLocked")))
	{
		lock(true);
	}
	else if (CFEqual(lock_unlock, CFSTR("com.apple.screenIsUnlocked")))
	{
		lock(false);
	}
}

bool OsEventHandlerMacOS::registerLockHandler()
{
	bool isRegistered{ _isLockRegistered };
	if (!_isLockRegistered)
	{
		CFNotificationCenterRef distCenter;

		distCenter = CFNotificationCenterGetDistributedCenter();
		if (distCenter != nullptr)
		{
			CFNotificationCenterAddObserver(distCenter, this, &OsEventHandlerMacOS::notificationCenterCallBack, lockSignal, nullptr, CFNotificationSuspensionBehaviorDeliverImmediately);
			CFNotificationCenterAddObserver(distCenter, this, &OsEventHandlerMacOS::notificationCenterCallBack, unlockSignal, nullptr, CFNotificationSuspensionBehaviorDeliverImmediately);
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
	if (_isLockRegistered)
	{
		CFNotificationCenterRemoveEveryObserver(CFNotificationCenterGetDistributedCenter(), this);
		_isLockRegistered = false;
	}
}

#endif
