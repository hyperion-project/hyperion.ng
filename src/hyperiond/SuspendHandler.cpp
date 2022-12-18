#include "SuspendHandler.h"

#include <hyperion/HyperionIManager.h>
#include <iostream>

SuspendHandlerBase::SuspendHandlerBase()
{
	connect(this, &SuspendHandlerBase::suspendEvent, HyperionIManager::getInstance(), &HyperionIManager::suspend);
	connect(this, &SuspendHandlerBase::resumeEvent, HyperionIManager::getInstance(), &HyperionIManager::resume);
	connect(this, &SuspendHandlerBase::locked, HyperionIManager::getInstance(), &HyperionIManager::toggleIdle);
}

SuspendHandlerBase::~SuspendHandlerBase()
{
}

void SuspendHandlerBase::suspend(bool sleep)
{
	if (sleep)
	{
		Info(Logger::getInstance("DAEMON"), "Suspend event - Hyperion is going to sleep");
		emit suspendEvent();
	}
	else
	{
		Info(Logger::getInstance("DAEMON"), "Resume event - Hyperion is going to wake up");
		emit resumeEvent();
	}
}

void SuspendHandlerBase::lock(bool isLocked)
{
	if (isLocked)
	{
		Info(Logger::getInstance("DAEMON"), "Screen locked");
		emit locked(isLocked);
	}
	else
	{
		Info(Logger::getInstance("DAEMON"), "Screen unlocked");
		emit locked(isLocked);
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
