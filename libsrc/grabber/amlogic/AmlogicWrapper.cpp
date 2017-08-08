// QT includes
#include <QDateTime>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Amlogic grabber includes
#include <grabber/AmlogicWrapper.h>
#include <grabber/AmlogicGrabber.h>


AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("AmLogic", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(new AmlogicGrabber(grabWidth, grabHeight))
{
	_ggrabber = _grabber;
}

AmlogicWrapper::~AmlogicWrapper()
{
	delete _grabber;
}

void AmlogicWrapper::action()
{
	updateOutputSize();
	if (_grabber->grabFrame(_image) >= 0)
	{
		setImage();
	}
}
