#include "SuspendHandler.h"

#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonObject>

#include <hyperion/HyperionIManager.h>
#include <iostream>

SuspendHandlerBase::SuspendHandlerBase()
	: _isSuspendEnabled(false)
	, _isLockEnabled(false)
	, _isSuspendOnLock(false)
	, _isSuspendRegistered(false)
	, _isLockRegistered(false)
	, _isSuspended(false)
	, _isIdle(false)
	, _isLocked (false)

{
	_log = Logger::getInstance("EVENTS");
}

SuspendHandlerBase::~SuspendHandlerBase()
{
	unregisterLockHandler();
	unregisterSuspendHandler();

	unregisterIdleApiHandler();
	unregisterSuspendApiHandler();
}

void SuspendHandlerBase::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::SYSTEMEVENTS)
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
			registerSuspendHandler();
		}
		else
		{
			unregisterSuspendHandler();
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

		//Handle API event related configurations
		_isSuspendApiEnabled = obj["suspendApiEnable"].toBool(true);
		if (_isSuspendApiEnabled)
		{
			// Listen to suspend/resume/idle events received by the OS
			registerSuspendApiHandler();
		}
		else
		{
			unregisterSuspendApiHandler();
		}

		_isIdleApiEnabled = obj["idleApiEnable"].toBool(true);
		if (_isIdleApiEnabled)
		{
			// Listen to lock/screensaver events received by the OS
			registerIdleApiHandler();
		}
		else
		{
			unregisterIdleApiHandler();
		}
	}
}

bool SuspendHandlerBase::registerSuspendHandler()
{
	Debug(_log, "_isSuspendRegistered: %d", _isSuspendRegistered);
	if (!_isSuspendRegistered)
	{
		connect(this, &SuspendHandlerBase::suspendEvent, HyperionIManager::getInstance(), &HyperionIManager::suspend);
		connect(this, &SuspendHandlerBase::resumeEvent, HyperionIManager::getInstance(), &HyperionIManager::resume);
		Info(_log, "Registered for suspend/resume events.");
		_isSuspendRegistered = true;
	}
	return true;
}

void SuspendHandlerBase::unregisterSuspendHandler()
{
	if (_isSuspendRegistered)
	{
		disconnect(this, &SuspendHandlerBase::suspendEvent, HyperionIManager::getInstance(), &HyperionIManager::suspend);
		disconnect(this, &SuspendHandlerBase::resumeEvent, HyperionIManager::getInstance(), &HyperionIManager::resume);
		Info(_log, "Unregistered for suspend/resume events.");
		_isSuspendRegistered = false;
	}
}

bool SuspendHandlerBase::registerLockHandler()
{
	disconnect(this, &SuspendHandlerBase::lockedEvent,nullptr, nullptr);
	if (_isSuspendOnLock)
	{
		connect(this, &SuspendHandlerBase::lockedEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleSuspend);
	}
	else
	{
		connect(this, &SuspendHandlerBase::lockedEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleIdle);
	}
	connect(this, &SuspendHandlerBase::idleEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleIdle);
	Info(_log, "Registered for lock/unlock events. %s on lock event.", _isSuspendOnLock ? "Suspend" : "Idle");
	_isLockRegistered = true;
	return true;
}

void SuspendHandlerBase::unregisterLockHandler()
{
	if (_isLockRegistered)
	{
		disconnect(this, &SuspendHandlerBase::lockedEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleSuspend);
		disconnect(this, &SuspendHandlerBase::lockedEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleIdle);
		disconnect(this, &SuspendHandlerBase::idleEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleIdle);
		Info(_log, "Unregistered for lock/unlock events.");
		_isLockRegistered = false;
	}
}


bool SuspendHandlerBase::registerSuspendApiHandler()
{
	if (!_isSuspendApiRegistered)
	{
		connect(HyperionIManager::getInstance(), &HyperionIManager::triggerSuspend, this, QOverload<bool>::of(&SuspendHandler::suspend));
		connect(HyperionIManager::getInstance(), &HyperionIManager::triggerToggleSuspend, this, &SuspendHandler::toggleSuspend);
		Info(_log, "Registered for suspend/resume API events.");
		_isSuspendApiRegistered = true;
	}
	return true;
}

void SuspendHandlerBase::unregisterSuspendApiHandler()
{
	if (_isSuspendApiRegistered)
	{
		disconnect(HyperionIManager::getInstance(), &HyperionIManager::triggerSuspend, this, QOverload<bool>::of(&SuspendHandler::suspend));
		disconnect(HyperionIManager::getInstance(), &HyperionIManager::triggerToggleSuspend, this, &SuspendHandler::toggleSuspend);
		Info(_log, "Unregistered for suspend/resume API events.");
		_isSuspendApiRegistered = false;
	}
}

bool SuspendHandlerBase::registerIdleApiHandler()
{
	if (!_isIdleApiRegistered)
	{
		connect(HyperionIManager::getInstance(), &HyperionIManager::triggerIdle, this, &SuspendHandler::idle);
		connect(HyperionIManager::getInstance(), &HyperionIManager::triggerToggleIdle, this, &SuspendHandler::toggleIdle);
		Info(_log, "Registered for idle API events.");
		_isIdleApiRegistered = true;
	}
	return true;
}

void SuspendHandlerBase::unregisterIdleApiHandler()
{
	if (_isIdleApiRegistered)
	{
		disconnect(HyperionIManager::getInstance(), &HyperionIManager::triggerIdle, this, &SuspendHandler::idle);
		disconnect(HyperionIManager::getInstance(), &HyperionIManager::triggerToggleIdle, this, &SuspendHandler::toggleIdle);
		Info(_log, "Unregistered for idle API events.");
		_isIdleApiRegistered = false;
	}
}

void SuspendHandlerBase::suspend()
{
	suspend(true);
}

void SuspendHandlerBase::suspend(bool sleep)
{
	if (sleep)
	{
		if (!_isSuspended)
		{
			_isSuspended = true;
			Info(_log, "Suspend event received - Hyperion is going to sleep");
			emit suspendEvent();
		}
		else
		{
			Debug(_log, "Suspend event ignored - already suspended");
		}
	}
	else
	{
		if (_isSuspended || _isIdle)
		{
			Info(_log, "Resume event received - Hyperion is going into working mode");
			emit resumeEvent();
			_isSuspended = false;
			_isIdle = false;
		}
		else
		{
			Debug(_log, "Resume event ignored - not in suspend nor idle mode");
		}
	}
}

void SuspendHandlerBase::resume()
{
	suspend(false);
}

void SuspendHandlerBase::toggleSuspend()
{
	Debug(_log, "Toggle suspend event received");
	if (!_isSuspended)
	{
		suspend(true);
	}
	else
	{
		suspend(false);
	}
}

void SuspendHandlerBase::idle(bool isIdle)
{
	if (!_isSuspended)
	{
		if (isIdle)
		{
			if (!_isIdle)
			{
				_isIdle = true;
				Info(_log, "Idle event received");
				emit idleEvent(isIdle);
			}
		}
		else
		{
			if (_isIdle)
			{
				Info(_log, "Resume from idle event recevied");
				emit idleEvent(isIdle);
				_isIdle = false;
			}
		}
	}
	else
	{
		Debug(_log, "Idle event ignored - Hyperion is suspended");
	}
}

void SuspendHandlerBase::toggleIdle()
{
	Debug(_log, "Toggle idle event received");
	if (!_isIdle)
	{
		idle(true);
	}
	else
	{
		idle(false);
	}
}

void SuspendHandlerBase::lock(bool isLocked)
{
	if (!_isSuspended)
	{
		if (isLocked)
		{
			if (!_isLocked)
			{
				_isLocked = true;
				Info(_log, "Screen lock event received");
				emit lockedEvent(isLocked);
			}
		}
		else
		{
			if (_isLocked)
			{
				Info(_log, "Screen unlock event received");
				emit lockedEvent(isLocked);
				_isLocked = false;
			}
		}
	}
	else
	{
		Debug(_log, "Screen lock event ignored - Hyperion is suspended");
	}
}

#if defined(_WIN32)
#include <QCoreApplication>
#include <QWidget>
#include <windows.h>
#include <wtsapi32.h>

#pragma comment( lib, "wtsapi32.lib" )

SuspendHandlerWindows::SuspendHandlerWindows()
{
}

SuspendHandlerWindows::~SuspendHandlerWindows()
{
	unregisterLockHandler();
	unregisterSuspendHandler();
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool SuspendHandlerWindows::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* /*result*/)
#else
bool SuspendHandlerWindows::nativeEventFilter(const QByteArray& eventType, void* message, long int* /*result*/)
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

bool SuspendHandlerWindows::registerSuspendHandler()
{
	bool isRegistered {false};
	if (!_isSuspendRegistered)
	{
		auto handle = reinterpret_cast<HWND> (_widget.winId());
		_notifyHandle = RegisterSuspendResumeNotification(handle, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (_notifyHandle != NULL)
		{
			QCoreApplication::instance()->installNativeEventFilter(this);
			isRegistered = SuspendHandlerBase::registerSuspendHandler();
		}
		else
		{
			Error(_log, "Could not register for suspend/resume events!");
		}
	}
	return isRegistered;
}

void SuspendHandlerWindows::unregisterSuspendHandler()
{
	if (_isSuspendRegistered)
	{
		if (_notifyHandle != NULL)
		{
			QCoreApplication::instance()->removeNativeEventFilter(this);
			UnregisterSuspendResumeNotification(_notifyHandle);
		}
		_notifyHandle = NULL;
		SuspendHandlerBase::unregisterSuspendHandler();
	}
}

bool SuspendHandlerWindows::registerLockHandler()
{
	bool isRegistered {false};
	if (!_isLockRegistered)
	{
		auto handle = reinterpret_cast<HWND> (_widget.winId());
		if (WTSRegisterSessionNotification(handle, NOTIFY_FOR_THIS_SESSION))
		{
			isRegistered = SuspendHandlerBase::registerLockHandler();
		}
		else
		{
			Error(_log, "Could not register for lock/unlock events!");
		}
	}
	return isRegistered;
}

void SuspendHandlerWindows::unregisterLockHandler()
{
	if (_isLockRegistered)
	{
		auto handle = reinterpret_cast<HWND> (_widget.winId());
		WTSUnRegisterSessionNotification(handle);
		SuspendHandlerBase::unregisterLockHandler();
	}
}

#elif defined(__linux__) && defined(HYPERION_HAS_DBUS)
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

SuspendHandlerLinux::SuspendHandlerLinux()
{
}

bool SuspendHandlerLinux::registerSuspendHandler()
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
			isRegistered = SuspendHandlerBase::registerSuspendHandler();
		}

	}
	return isRegistered;
}

void SuspendHandlerLinux::unregisterSuspendHandler()
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
				SuspendHandlerBase::unregisterSuspendHandler();
			}
			else
			{
				Error(_log, "Could not unregister for suspend/resume events via service: %s", QSTRING_CSTR(service));
			}
		}
	}
}

bool SuspendHandlerLinux::registerLockHandler()
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
		isRegistered = SuspendHandlerBase::registerLockHandler();
	}

	return isRegistered;
}

void SuspendHandlerLinux::unregisterLockHandler()
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
				SuspendHandlerBase::unregisterLockHandler();
			}
		}
	}
}


#endif
