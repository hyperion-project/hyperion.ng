#include "events/EventScheduler.h"

#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

#include <events/EventHandler.h>
#include <utils/Logger.h>

EventScheduler::EventScheduler()
	: _isEnabled(false)
{
	TRACK_SCOPE();
	qRegisterMetaType<Event>("Event");
	_log = Logger::getInstance("EVENTS-SCHED");

	QObject::connect(this, &EventScheduler::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);

	Debug(_log, "Hyperion event scheduler created");
}

EventScheduler::~EventScheduler()
{
	TRACK_SCOPE();
	QObject::disconnect(this, &EventScheduler::signalEvent, EventHandler::getInstance().data(), &EventHandler::handleEvent);
	clearTimers();
	Info(_log, "Hyperion event scheduler stopped");
}

void EventScheduler::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type != settings::SCHEDEVENTS)
	{
		return;
	}
	const QJsonObject& obj = config.object();

	_isEnabled = obj["enable"].toBool(false);
	Debug(_log, "Event scheduler is %s", _isEnabled? "enabled" : "disabled");

	if (!_isEnabled)
	{
		disable();
		return;
	}

	if (!obj.contains("actions"))
	{
		return;
	}

	_scheduledEvents.clear();

	const QJsonArray actionItems = obj["actions"].toArray();
	if (!actionItems.isEmpty())
	{
		timeEvent timeEvent;
		for (const QJsonValue &item : actionItems)
		{
			QString action = item.toObject().value("action").toString();
			timeEvent.action = stringToEvent(action);

			QString event =  item.toObject().value("event").toString();
			timeEvent.time = QTime::fromString(event,"hh:mm");
			if (timeEvent.time.isValid())
			{
				_scheduledEvents.append(timeEvent);
				Debug(_log, "Time-Event : \"%s\" linked to action \"%s\"", QSTRING_CSTR(event), QSTRING_CSTR(action));
			}
			else
			{
				Error(_log, "Error in configured time : \"%s\" linked to action \"%s\"", QSTRING_CSTR(event), QSTRING_CSTR(action));
			}
		}
	}

	if (!_scheduledEvents.isEmpty())
	{
		enable();
	}
	else
	{
		Warning(_log, "No scheduled events to listen to are configured currently.");
	}

}

bool EventScheduler::enable()
{
	bool enabled {false};

	clearTimers();
	for (int i = 0; i < _scheduledEvents.size(); ++i)
	{
		auto* timer = new QTimer(this);
		timer->setTimerType(Qt::PreciseTimer);
		_timers.append(timer);

		// Calculate the milliseconds until the next occurrence of the scheduled time
		int milliseconds = getMillisecondsToNextScheduledTime(_scheduledEvents.at(i).time);
		timer->start(milliseconds);

		QObject::connect(timer, &QTimer::timeout, this, [this, i]() { handleEvent(i); });
	}

	enabled = true;
	Info(_log, "Event scheduler started");

	return enabled;
}

void EventScheduler::disable()
{
	Info(_log, "Disabling event scheduler");
	clearTimers();
}

void EventScheduler::clearTimers()
{
	for (QTimer *timer :  std::as_const(_timers)) {
		timer->disconnect();
		timer->stop();
		delete timer;
	}
	_timers.clear();
}

void EventScheduler::handleEvent(int timerIndex)
{
	QTime time = _scheduledEvents.at(timerIndex).time;
	Event action = _scheduledEvents.at(timerIndex).action;
	Debug(_log, "Event : \"%s\" triggers action \"%s\"", QSTRING_CSTR(time.toString()), eventToString(action) );
	if ( action != Event::Unknown )
	{
		emit signalEvent(action);
	}

	// Retrigger the timer for the next occurrence
	QTimer* timer = _timers.at(timerIndex);
	auto milliseconds = getMillisecondsToNextScheduledTime(time);
	timer->start(milliseconds);
}

qint64 EventScheduler::getMillisecondsToNextScheduledTime(const QTime& scheduledTime) const
{
	QDateTime currentDateTime = QDateTime::currentDateTime();
	QTime currentTime = currentDateTime.time();

	QDateTime nextOccurrence = currentDateTime;
	nextOccurrence.setTime(scheduledTime);

	// If the scheduled time has already passed for today, schedule it for tomorrow
	if (currentTime > scheduledTime)
	{
		nextOccurrence = nextOccurrence.addDays(1);
	}

	auto milliseconds = currentDateTime.msecsTo(nextOccurrence);
	return (milliseconds > 0) ? milliseconds : 0;
}
