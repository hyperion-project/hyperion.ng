#include <events/EventHandler.h>

#include <QJsonDocument>
#include <QJsonObject>

#include <utils/Logger.h>
#include <utils/Process.h>
#include <hyperion/HyperionIManager.h>

Q_LOGGING_CATEGORY(event_hyperion, "hyperion.event.hyperion");

QScopedPointer<EventHandler> EventHandler::instance;

EventHandler::EventHandler()
	: _isSuspended(false)
	, _isIdle(false)
{
	TRACK_SCOPE();
	qRegisterMetaType<Event>("Event");
	_log = Logger::getInstance("EVENTS");
	if (auto mgrStrong = HyperionIManager::getInstanceWeak().toStrongRef())
	{
		QObject::connect(this, &EventHandler::signalEvent, mgrStrong.get(), &HyperionIManager::handleEvent);
	}
	Debug(_log, "Hyperion event handler created");
}

EventHandler::~EventHandler()
{
	TRACK_SCOPE();
	if (auto mgrStrong = HyperionIManager::getInstanceWeak().toStrongRef())
	{
		QObject::disconnect(this, &EventHandler::signalEvent, mgrStrong.get(), &HyperionIManager::handleEvent);
	}
}

QScopedPointer<EventHandler>& EventHandler::getInstance()
{
	if (!instance)
	{
		instance.reset(new EventHandler());
	}

	return instance;
}

void EventHandler::destroyInstance()
{
	if (instance)
	{
		instance.reset();
	}
}

void EventHandler::suspend()
{
	suspend(true);
}

void EventHandler::suspend(bool sleep)
{
	if (sleep)
	{
		if (!_isSuspended)
		{
			_isSuspended = true;
			Info(_log, "Suspend event received - Hyperion is going to sleep");
			emit signalEvent(Event::Suspend);
		}
		else
		{
			Debug(_log, "Suspend event ignored - already suspended");
		}
	}
	else
	{
		if (_isSuspended)
		{
			Info(_log, "Resume event received - Hyperion is going into working mode");
			_isSuspended = false;
			emit signalEvent(Event::Resume);
		}
		else
		{
			Debug(_log, "Resume event ignored - not in suspend nor idle mode");
		}
	}
}

void EventHandler::resume()
{
	suspend(false);
}

void EventHandler::toggleSuspend()
{
	Debug(_log, "Toggle suspend event received");
	if (!_isSuspended)
	{
		suspend(true);
	}
	else
	{
		suspend(false);
	}
}

void EventHandler::idle()
{
	idle(true);
}

void EventHandler::idle(bool isIdle)
{
	if (!_isSuspended)
	{
		if (isIdle)
		{
			if (!_isIdle)
			{
				_isIdle = true;
				Info(_log, "Idle event received");
				emit signalEvent(Event::Idle);
			}
		}
		else
		{
			if (_isIdle)
			{
				Info(_log, "Resume from idle event recevied");
				_isIdle = false;
				emit signalEvent(Event::ResumeIdle);
			}
		}
	}
	else
	{
		Debug(_log, "Idle event ignored - Hyperion is suspended");
	}
}
void EventHandler::resumeIdle()
{
	idle(false);
}

void EventHandler::toggleIdle()
{
	Debug(_log, "Toggle idle event received");
	if (!_isIdle)
	{
		idle(true);
	}
	else
	{
		idle(false);
	}
}

void EventHandler::handleEvent(Event event)
{
	QObject *senderObj = QObject::sender();
	QString senderObjectClass;
	if (senderObj != nullptr)
	{
		senderObjectClass = senderObj->metaObject()->className();
	} else
	{
		senderObjectClass = "unknown sender";
	}
	Debug(_log,"%s Event [%d] received from %s", eventToString(event), event, QSTRING_CSTR(senderObjectClass));

	switch (event) {
	case Event::Suspend:
		suspend();
		break;

	case Event::Resume:
		resume();
		break;

	case Event::ToggleSuspend:
		toggleSuspend();
		break;

	case Event::Idle:
		idle(true);
		break;

	case Event::ResumeIdle:
		idle(false);
		break;

	case Event::ToggleIdle:
		toggleIdle();
		break;

	case Event::Reload:
		emit signalEvent(Event::Reload);
		Process::restartHyperion(10);
		break;

	case Event::Restart:
		emit signalEvent(Event::Restart);
		Process::restartHyperion(11);
		break;

	case Event::Quit:
		emit signalEvent(Event::Quit);
		break;

	default:
		Error(_log,"Unkonwn Event '%d' received", event);
		break;
	}
}
