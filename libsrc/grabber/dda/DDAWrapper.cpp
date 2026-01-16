#include "grabber/dda/DDAWrapper.h"
#include "events/EventHandler.h"


DDAWrapper::DDAWrapper(int updateRate_Hz, int display, int pixelDecimation, int cropLeft, int cropRight, int cropTop,
	int cropBottom)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz), _grabber(display, cropLeft, cropRight, cropTop, cropBottom)

{
	_grabber.setPixelDecimation(pixelDecimation);
}

DDAWrapper::DDAWrapper(const QJsonDocument& grabberConfig)
	: DDAWrapper(GrabberWrapper::DEFAULT_RATE_HZ, 0, GrabberWrapper::DEFAULT_PIXELDECIMATION, 0, 0, 0, 0)
{
	if (_grabber.isAvailable(true))
	{
		this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

void DDAWrapper::handleSettingsUpdate(settings::type type, const QJsonDocument& grabberConfig)
{
	if (type != settings::SYSTEMCAPTURE)
	{
		return;
	}
	GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

bool DDAWrapper::open()
{
	if (_isScreenLocked)
		{
		qCDebug(grabber_screen_flow) << "Screen is locked - do not open grabber" << _grabber.getGrabberName();
		return false;
	}

	qCDebug(grabber_screen_flow) << "Open grabber" << _grabber.getGrabberName() << "currently:" << (_grabber.isEnabled() ? "enabled" : "disabled");
	return _grabber.resetDeviceAndCapture();
}

bool DDAWrapper::start()
{
	qCDebug(grabber_screen_flow) << "Start grabber" << _grabber.getGrabberName() << "currently:" << (_grabber.isEnabled() ? "enabled" : "disabled");
	return GrabberWrapper::start();
}

//bool DDAWrapper::start()
//{
//	qCDebug(grabber_screen_flow) << "Start grabber" << _grabber.getGrabberName() << "currently:" << (_grabber.isEnabled() ? "enabled" : "disabled");
//
//	if (_grabber.isAvailable() && _grabber.isEnabled())
//	{
//		if (isActive())
//		{
//			qCDebug(grabber_screen_flow) << "Grabber" << _grabber.getGrabberName() << "is already running";
//			return true;
//		}
//
//		qDebug(grabber_screen_flow) << "Start grabber" << _grabber.getGrabberName()
//			<< ". System is" << (OsEventHandler::getInstance()->isLocked() ? "locked" : "unlocked")
//			<< "and" << (OsEventHandler::getInstance()->isSuspended() ? "suspended" : "resumed");
//
//		if (OsEventHandler::getInstance()->isLocked())
//		{
//			qCDebug(grabber_screen_flow) << "Grabber" << _grabber.getGrabberName() << "not started, as system is still locked";
//			return false;
//		}
//
//		if (_grabber.resetDeviceAndCapture())
//		{
//			return GrabberWrapper::start();
//		}
//	}
//
//	return false;
//}

void DDAWrapper::action()
{
	if (!_grabber.isAvailable())
	{
		return;
	}

	transferFrame(_grabber);
}

void DDAWrapper::handleEvent(Event event)
{
	if (!_grabber.isEnabled())
	{
		qCDebug(grabber_screen_flow) << "Grabber is disabled. Ignore events.";
		return;
	}

	qDebug(grabber_screen_flow) << "Handle event" << event << "for grabber" << _grabber.getGrabberName()
		<< ". Screen is" << (_isScreenLocked ? "locked" : "unlocked");

	switch (event)
	{
	case Event::Lock:
		_isScreenLocked = true;
		qCDebug(grabber_screen_flow) << "Screen is Locked - Remember state";
		break;
	case Event::Unlock:
		_isScreenLocked = false;
		qCDebug(grabber_screen_flow) << "Resume after screen was unlocked - Start Grabber";
		start();
		break;
	case Event::Resume:
		if (!_isScreenLocked)
		{
			qCDebug(grabber_screen_flow) << "Resume from Suspend - Start Grabber as system is not locked";
			start();
		}
		else
		{
			qDebug(grabber_screen_flow) << "Resume from Suspend - Grabber not started, as system is still locked";
		}
		break;

	case Event::Idle:
	case Event::Suspend:
	default:
		break;
	}
}
