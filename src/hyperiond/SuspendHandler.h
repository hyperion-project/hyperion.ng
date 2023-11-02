#ifndef SUSPENDHANDLER_H
#define SUSPENDHANDLER_H
#include <QObject>
#include <QJsonDocument>

#if defined(_WIN32)
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>
#include <QWidget>
#include <windows.h>
#endif

#include <utils/settings.h>

class Logger;

class SuspendHandlerBase : public QObject {
	Q_OBJECT

public:
	SuspendHandlerBase();
	~SuspendHandlerBase() override;

public slots:

	void suspend(bool sleep);

	void suspend();
	void resume();
	void toggleSuspend();

	void idle(bool isIdle);
	void toggleIdle();

	void lock(bool isLocked);

	virtual void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

signals:

	void suspendEvent();
	void resumeEvent();
	void lockedEvent(bool);
	void idleEvent(bool);

protected:

	virtual bool registerSuspendHandler();
	virtual void unregisterSuspendHandler();
	virtual bool registerLockHandler();
	virtual void unregisterLockHandler();

	virtual bool registerSuspendApiHandler();
	virtual void unregisterSuspendApiHandler();
	virtual bool registerIdleApiHandler();
	virtual void unregisterIdleApiHandler();

	bool _isSuspendEnabled;
	bool _isLockEnabled;
	bool _isSuspendApiEnabled;
	bool _isIdleApiEnabled;
	bool _isSuspendOnLock;

	bool _isSuspendRegistered;
	bool _isLockRegistered;
	bool _isSuspendApiRegistered;
	bool _isIdleApiRegistered;

	Logger * _log {};

private:

	bool _isSuspended;
	bool _isIdle;
	bool _isLocked;
};

#if defined(_WIN32)

class SuspendHandlerWindows : public SuspendHandlerBase, public QAbstractNativeEventFilter {

public:
	SuspendHandlerWindows();
	~SuspendHandlerWindows() override;

protected:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
#else
	bool nativeEventFilter(const QByteArray& eventType, void* message, long int* result) override;
#endif

private:

	bool registerSuspendHandler() override;
	void unregisterSuspendHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;

	QWidget			_widget;
	HPOWERNOTIFY	_notifyHandle;
};

using SuspendHandler = SuspendHandlerWindows;

#elif defined(__linux__) && defined(HYPERION_HAS_DBUS)

class SuspendHandlerLinux : public SuspendHandlerBase {
	Q_OBJECT

public:
	SuspendHandlerLinux();

private:
	bool registerSuspendHandler() override;
	void unregisterSuspendHandler() override;
	bool registerLockHandler() override;
	void unregisterLockHandler() override;
};

using SuspendHandler = SuspendHandlerLinux;

#else
using SuspendHandler = SuspendHandlerBase;
#endif

#endif // SUSPENDHANDLER_H
