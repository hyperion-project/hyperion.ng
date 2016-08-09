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

		Info(_log, "change state to %s", (enable ? "enabled" : "disabled") );
	}
}
