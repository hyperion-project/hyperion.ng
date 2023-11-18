#ifndef OSEVENTHANDLER_H
#define OSEVENTHANDLER_H
#include <QObject>
#include <QJsonDocument>

#include <events/EventEnum.h>

#if defined(_WIN32)
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>
#include <QWidget>
#include <windows.h>
#endif

#include <utils/settings.h>

class Logger;

class OsEventHandlerBase : public QObject
{
	Q_OBJECT

public:
	OsEventHandlerBase();
	~OsEventHandlerBase() override;

public slots:
	void suspend(bool sleep);
	void lock(bool isLocked);

	virtual void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

signals:
	void signalEvent(Event event);

protected:
	virtual bool registerOsEventHandler() { return true; }
	virtual void unregisterOsEventHandler() {}
	virtual bool registerLockHandler() { return true; }
	virtual void unregisterLockHandler() {}

	bool _isSuspendEnabled;
	bool _isLockEnabled;
	bool _isSuspendOnLock;

	bool _isSuspendRegistered;
	bool _isLockRegistered;

	Logger * _log {};
};

#if defined(_WIN32)

class OsEventHandlerWindows : public OsEventHandlerBase, public QAbstractNativeEventFilter
{

public:
	OsEventHandlerWindows();
	~OsEventHandlerWindows() override;

protected:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
#else
	bool nativeEventFilter(const QByteArray& eventType, void* message, long int* result) override;
#endif

private:
	bool registerOsEventHandler() override;
	void unregisterOsEventHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;

	QWidget			_widget;
	HPOWERNOTIFY	_notifyHandle;
};

using OsEventHandler = OsEventHandlerWindows;

#elif defined(__linux__)
class OsEventHandlerLinux : public OsEventHandlerBase
{
	Q_OBJECT

	static void static_signaleHandler(int signum)
	{
		OsEventHandlerLinux::getInstance()->handleSignal(signum);
	}

public:
	OsEventHandlerLinux();

	void handleSignal (int signum);

private:
	static OsEventHandlerLinux* getInstance();

#if defined(HYPERION_HAS_DBUS)
	bool registerOsEventHandler() override;
	void unregisterOsEventHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;
#endif
};

using OsEventHandler = OsEventHandlerLinux;

#elif defined(__APPLE__)
class OsEventHandlerMacOS : public OsEventHandlerBase
{
	Q_OBJECT

public:
	OsEventHandlerMacOS();

private:
	bool registerOsEventHandler() override;
	void unregisterOsEventHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;

	void *_sleepEventHandler;
	void *_lockEventHandler;
};

using OsEventHandler = OsEventHandlerMacOS;

#else
using OsEventHandler = OsEventHandlerBase;
#endif

#endif // OSEVENTHANDLER_H
