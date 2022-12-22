#include "SuspendHandler.h"

#include <QtGlobal>

#include <hyperion/HyperionIManager.h>
#include <iostream>

SuspendHandlerBase::SuspendHandlerBase()
	: _isSuspended(false)
	, _isIdle(false)
	, _isLocked (false)
{
	// Trigger suspend/resume/idle scenarios to be executed by Instance mMnager
	connect(this, &SuspendHandlerBase::suspendEvent, HyperionIManager::getInstance(), &HyperionIManager::suspend);
	connect(this, &SuspendHandlerBase::resumeEvent, HyperionIManager::getInstance(), &HyperionIManager::resume);
	connect(this, &SuspendHandlerBase::lockedEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleIdle);
	connect(this, &SuspendHandlerBase::idleEvent, HyperionIManager::getInstance(), &HyperionIManager::toggleIdle);


	// Listen to suspend/resume/idle events received by the Instance Manager via API
	connect(HyperionIManager::getInstance(), &HyperionIManager::triggerSuspend, this, QOverload<bool>::of(&SuspendHandler::suspend));
	connect(HyperionIManager::getInstance(), &HyperionIManager::triggerToggleSuspend, this, &SuspendHandler::toggleSuspend);
	connect(HyperionIManager::getInstance(), &HyperionIManager::triggerIdle, this, &SuspendHandler::idle);
	connect(HyperionIManager::getInstance(), &HyperionIManager::triggerToggleIdle, this, &SuspendHandler::toggleIdle);
}

SuspendHandlerBase::~SuspendHandlerBase()
{
}

void SuspendHandlerBase::suspend()
{
	Debug(Logger::getInstance("DAEMON"), "");
	suspend(true);
}

void SuspendHandlerBase::suspend(bool sleep)
{
	if (sleep)
	{
		if (!_isSuspended)
		{
			_isSuspended = true;
			Info(Logger::getInstance("DAEMON"), "Suspend event received - Hyperion is going to sleep");
			emit suspendEvent();
		}
		else
		{
			Debug(Logger::getInstance("DAEMON"), "Suspend event ignored - already suspended");
		}
	}
	else
	{
		if (_isSuspended || _isIdle)
		{
			Info(Logger::getInstance("DAEMON"), "Resume event received - Hyperion is going into working mode");
			emit resumeEvent();
			_isSuspended = false;
			_isIdle = false;
		}
		else
		{
			Debug(Logger::getInstance("DAEMON"), "Resume event ignored - not in suspend nor idle mode");
		}
	}
}

void SuspendHandlerBase::resume()
{
	Debug(Logger::getInstance("DAEMON"), "");
	suspend(false);
}

void SuspendHandlerBase::toggleSuspend()
{
	Debug(Logger::getInstance("DAEMON"), "Toggle suspend event received");
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
				Info(Logger::getInstance("DAEMON"), "Idle event received");
				emit idleEvent(isIdle);
			}
		}
		else
		{
			if (_isIdle)
			{
				Info(Logger::getInstance("DAEMON"), "Resume from idle event recevied");
				emit idleEvent(isIdle);
				_isIdle = false;
			}
		}
	}
	else
	{
		Debug(Logger::getInstance("DAEMON"), "Idle event ignored - Hyperion is suspended");
	}
}

void SuspendHandlerBase::toggleIdle()
{
	Debug(Logger::getInstance("DAEMON"), "Toggle idle event received");
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
				Info(Logger::getInstance("DAEMON"), "Screen lock event received");
				emit lockedEvent(isLocked);
			}
		}
		else
		{
			if (_isLocked)
			{
				Info(Logger::getInstance("DAEMON"), "Screen unlock event received");
				emit lockedEvent(isLocked);
				_isLocked = false;
			}
		}
	}
	else
	{
		Debug(Logger::getInstance("DAEMON"), "Screen lock event ignored - Hyperion is suspended");
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
	auto handle = reinterpret_cast<HWND> (_widget.winId());

	_notifyHandle = RegisterSuspendResumeNotification(handle, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (_notifyHandle != NULL)
	{
		QCoreApplication::instance()->installNativeEventFilter(this);
	}
	else
	{
		Error(Logger::getInstance("DAEMON"), "Could not register for suspend/resume events!");
	}

	if (!WTSRegisterSessionNotification(handle, NOTIFY_FOR_THIS_SESSION))
	{
		Error(Logger::getInstance("DAEMON"), "Could not register for lock/unlock events!");
	}
}

SuspendHandlerWindows::~SuspendHandlerWindows()
{
	if (_notifyHandle != NULL)
	{
		QCoreApplication::instance()->removeNativeEventFilter(this);

		UnregisterSuspendResumeNotification(_notifyHandle);

		auto handle = reinterpret_cast<HWND> (_widget.winId());
		WTSUnRegisterSessionNotification(handle);
	}
	_notifyHandle = NULL;
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
	QDBusConnection systemBus = QDBusConnection::systemBus();
	if (!systemBus.isConnected())
	{
		Error(Logger::getInstance("DAEMON"), "Suspend/resume handler - System bus is not connected");
	}
	else
	{
		QString service = dbusSignals.find("Suspend").value().service;
		if (!systemBus.connect(service,
							   dbusSignals.find("Suspend").value().path,
							   dbusSignals.find("Suspend").value().interface,
							   dbusSignals.find("Suspend").value().name,
							   this, SLOT(suspend(bool))))
			Error(Logger::getInstance("DAEMON"), "Could not register for suspend/resume events [%s]!", QSTRING_CSTR(service));
		else
		{
			Debug(Logger::getInstance("DAEMON"), "Registered for suspend/resume events [%s].", QSTRING_CSTR(service));
		}
	}

	QDBusConnection sessionBus = QDBusConnection::sessionBus();
	if (!sessionBus.isConnected())
	{
		Error(Logger::getInstance("DAEMON"), "Lock/unlock handler- Session bus is not connected");
	}
	else
	{
		DbusSignalsMap::const_iterator iter = dbusSignals.find("ScreenSaver");
		while (iter != dbusSignals.end() && iter.key() == "ScreenSaver") {
			QString service = iter.value().service;
			if (!sessionBus.connect(service,
									iter.value().path,
									iter.value().interface,
									iter.value().name,
									this, SLOT(lock(bool))))
				Error(Logger::getInstance("DAEMON"), "Could not register for lock/unlock events [%s]!", QSTRING_CSTR(service));
			else
			{
				Debug(Logger::getInstance("DAEMON"), "Registered for lock/unlock events [%s].", QSTRING_CSTR(service));
			}
			++iter;
		}
	}
}
#endif
