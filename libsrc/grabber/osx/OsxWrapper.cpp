// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Osx grabber includes
#include <grabber/OsxWrapper.h>
#include <grabber/OsxFrameGrabber.h>

OsxWrapper::OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("OSX FrameGrabber", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(new OsxFrameGrabber(display, grabWidth, grabHeight))
{
	// Configure the timer to generate events every n milliseconds
	_ggrabber = _grabber;
}

OsxWrapper::~OsxWrapper()
{
	delete _grabber;
}

void OsxWrapper::action()
{
	updateOutputSize();
	_grabber->grabFrame(_image);
	setImage();
}
