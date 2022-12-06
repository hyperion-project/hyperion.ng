#include "SuspendHandler.h"

#include <hyperion/HyperionIManager.h>
#include <iostream>

SuspendHandlerBase::SuspendHandlerBase()
{
	connect(this, &SuspendHandlerBase::suspendEvent, HyperionIManager::getInstance(), &HyperionIManager::suspend);
	connect(this, &SuspendHandlerBase::resumeEvent, HyperionIManager::getInstance(), &HyperionIManager::resume);
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

#if defined(_WIN32)
#include <QCoreApplication>
#include <QWidget>
#include <windows.h>

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
}

SuspendHandlerWindows::~SuspendHandlerWindows()
{
	if (_notifyHandle != NULL)
	{
		QCoreApplication::instance()->removeNativeEventFilter(this);
		UnregisterSuspendResumeNotification(_notifyHandle);
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

	if (msg->message == WM_POWERBROADCAST)
	{
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
	}
	return false;
}

#elif defined(__linux__) && defined(HYPERION_HAS_DBUS)
#include <QDBusConnection>

// Constants
namespace {
	const QString UPOWER_SERVICE = QStringLiteral("org.freedesktop.login1");
	const QString UPOWER_PATH = QStringLiteral("/org/freedesktop/login1");
	const QString UPOWER_INTER = QStringLiteral("org.freedesktop.login1.Manager");
} //End of constants

SuspendHandlerLinux::SuspendHandlerLinux()
{
	QDBusConnection bus = QDBusConnection::systemBus();
	if (!bus.isConnected())
	{
		Error(Logger::getInstance("DAEMON"), "Suspend/resume - System bus is not connected");
		return;
	}

	if (!bus.connect(UPOWER_SERVICE, UPOWER_PATH, UPOWER_INTER, "PrepareForSleep", this, SLOT(suspend(bool))))
		Error(Logger::getInstance("DAEMON"), "Could not register for suspend/resume events!");
	else
	{
		Debug(Logger::getInstance("DAEMON"), "Registered for suspend/resume events");
	}
}
#endif
