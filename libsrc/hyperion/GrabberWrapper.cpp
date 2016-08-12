// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/GrabberWrapper.h>


GrabberWrapper::GrabberWrapper(std::string grabberName, const int priority)
	: _grabberName(grabberName)
	, _hyperion(Hyperion::getInstance())
	, _priority(priority)
	, _timer()
	, _log(Logger::getInstance(grabberName.c_str()))
	, _forward(true)
	, _processor(ImageProcessorFactory::getInstance().newImageProcessor())
{
	_timer.setSingleShot(false);

	_forward = _hyperion->getForwarder()->protoForwardingEnabled();
	connect(_hyperion, SIGNAL(componentStateChanged(hyperion::Components,bool)), this, SLOT(componentStateChanged(hyperion::Components,bool)));
	connect(&_timer, SIGNAL(timeout()), this, SLOT(action()));

}

GrabberWrapper::~GrabberWrapper()
{
	delete _processor;
}

bool GrabberWrapper::start()
{
	// Start the timer with the pre configured interval
	_timer.start();
	_hyperion->registerPriority(_grabberName,_priority);
	return _timer.isActive();

}

void GrabberWrapper::stop()
{
	// Stop the timer, effectivly stopping the process
	_timer.stop();
	_hyperion->unRegisterPriority(_grabberName);
}

void GrabberWrapper::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == hyperion::COMP_GRABBER && _timer.isActive() != enable)
	{
		if (enable) start();
		else        stop();

		_forward = _hyperion->getForwarder()->protoForwardingEnabled();

		if ( enable == _timer.isActive() )
		{
			Info(_log, "grabber change state to %s", (_timer.isActive() ? "enabled" : "disabled") );
		}
		else
		{
			WarningIf( enable, _log, "enable grabber failed");
		}
	}

	if (component == hyperion::COMP_BLACKBORDER && _processor->blackBorderDetectorEnabled() != enable)
	{
		_processor->enableBlackBorderDetector(enable);
		Info(_log, "bb detector change state to %s", (_processor->blackBorderDetectorEnabled() ? "enabled" : "disabled") );
	}
}

void GrabberWrapper::kodiPlay()
{
	start();
}

void GrabberWrapper::kodiPause()
{
	start();
}

void GrabberWrapper::kodiOff()
{
	stop();
}


void GrabberWrapper::setGrabbingMode(const GrabbingMode mode)
{
	switch (mode)
	{
	case GRABBINGMODE_VIDEO:
	case GRABBINGMODE_PAUSE:
		kodiPause();
		break;
	case GRABBINGMODE_AUDIO:
	case GRABBINGMODE_PHOTO:
	case GRABBINGMODE_MENU:
	case GRABBINGMODE_SCREENSAVER:
	case GRABBINGMODE_INVALID:
		kodiPlay();
		break;
	case GRABBINGMODE_OFF:
		kodiOff();
		break;
	}
}

