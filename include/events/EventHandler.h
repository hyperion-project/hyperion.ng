#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <utils/settings.h>
#include <events/EventEnum.h>

#include <QObject>
#include <QSharedPointer>
#include <QScopedPointer>


class Logger;

class EventHandler : public QObject
{
	Q_OBJECT

public:
	~EventHandler() override;

	static QScopedPointer<EventHandler>& getInstance();
	static void destroyInstance();

public slots:

	void suspend(bool sleep);

	void suspend();
	void resume();
	void toggleSuspend();

	void idle(bool isIdle);
	void idle();
	void resumeIdle();
	void toggleIdle();

	void handleEvent(Event event);

signals:
	void signalEvent(Event event);

protected:
	QSharedPointer<Logger> _log;

private:
	EventHandler();
	EventHandler(const EventHandler&) = delete;
	EventHandler& operator=(const EventHandler&) = delete;

	static QScopedPointer<EventHandler> instance;

	bool _isSuspended;
	bool _isIdle;
};


#endif // EVENTHANDLER_H

