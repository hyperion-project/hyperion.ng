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

bool DDAWrapper::start()
{
	if (_grabber.isAvailable())
	{
		if (_grabber.resetDeviceAndCapture())
		{
			return GrabberWrapper::start();
		}
	}

	return false;
}

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
	switch (event)
	{
	case Event::ResumeIdle:
		qCDebug(grabber_screen_flow) << "Resume from Idle - Start Grabber";
		start();
		_grabber.setEnabled(true);
		break;
	case Event::Resume:
		if (!EventHandler::getInstance()->isIdle())
		{
			qCDebug(grabber_screen_flow) << "Resume from Suspend - Start Grabber as not being in Idle";
			start();
			_grabber.setEnabled(true);
		}
		else
		{
			qCDebug(grabber_screen_flow) << "Resume from Suspend - Grabber not started, as Hyperion is still in Idle mode";
		}
		break;
	case Event::Idle:
	case Event::Suspend:
	default:
		break;
	}
}
