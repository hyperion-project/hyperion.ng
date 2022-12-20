#ifndef SUSPENDHANDLER_H
#define SUSPENDHANDLER_H
#include <QObject>

class SuspendHandlerBase : public QObject {
	Q_OBJECT

public:
	SuspendHandlerBase();
	virtual ~SuspendHandlerBase() override;

public slots:

	void suspend(bool sleep);

	void suspend();
	void resume();
	void toggleSuspend();

	void idle(bool isIdle);
	void toggleIdle();

	void lock(bool isLocked);

signals:

	void suspendEvent();
	void resumeEvent();
	void lockedEvent(bool);
	void idleEvent(bool);

private:

	bool _isSuspended;
	bool _isIdle;
	bool _isLocked;
};

#if defined(_WIN32)
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>
#include <QWidget>
#include <windows.h>

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
	QWidget			_widget;
	HPOWERNOTIFY	_notifyHandle;
};

using SuspendHandler = SuspendHandlerWindows;

#elif defined(__linux__) && defined(HYPERION_HAS_DBUS)

class SuspendHandlerLinux : public SuspendHandlerBase {

public:
	SuspendHandlerLinux();
};

using SuspendHandler = SuspendHandlerLinux;

#else
using SuspendHandler = SuspendHandlerBase;
#endif

#endif // SUSPENDHANDLER_H
