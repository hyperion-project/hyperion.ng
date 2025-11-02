#ifndef EVENTSCHEDULER_H
#define EVENTSCHEDULER_H
#include <QObject>
#include <QJsonDocument>
#include <QTime>
#include <QTimer>

#include <events/EventEnum.h>
#include <utils/settings.h>

class Logger;

class EventScheduler : public QObject
{
	Q_OBJECT

public:
	EventScheduler();
	~EventScheduler() override;

	virtual void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

signals:
	void signalEvent(Event event);

private slots:

	void handleEvent(int timerIndex);

private:

	struct timeEvent
	{
		QTime time;
		Event action;
	};

	bool enable();
	void disable();

	qint64 getMillisecondsToNextScheduledTime(const QTime& time) const;

	void clearTimers();

	QJsonDocument _config;

	bool _isEnabled;

	QList<timeEvent> _scheduledEvents;
	QList<QTimer*> _timers;

	QSharedPointer<Logger> _log;
};

#endif // EVENTSCHEDULER_H
