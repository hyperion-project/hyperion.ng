#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <utils/settings.h>
#include <events/Event.h>

#include <QObject>

class Logger;

class EventHandler : public QObject {
	Q_OBJECT

public:

	EventHandler();
	~EventHandler() override;

	static EventHandler* getInstance();

public slots:

	virtual void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

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

	Logger * _log {};

private:

	bool _isSuspended;
	bool _isIdle;
};


#endif // EVENTHANDLER_H

