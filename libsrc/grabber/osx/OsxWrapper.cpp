// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Osx grabber includes
#include <grabber/OsxWrapper.h>
#include <grabber/OsxFrameGrabber.h>

OsxWrapper::OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("OSX FrameGrabber", priority)
	, _updateInterval_ms(1000/updateRate_Hz)
	, _timeout_ms(2 * _updateInterval_ms)
	, _image(grabWidth, grabHeight)
	, _grabber(new OsxFrameGrabber(display, grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);

	_processor->setSize(grabWidth, grabHeight);
}

OsxWrapper::~OsxWrapper()
{
	delete _grabber;
}

void OsxWrapper::action()
{
	// Grab frame into the allocated image
	_grabber->grabFrame(_image);

	emit emitImage(_priority, _image, _timeout_ms);

	_processor->process(_image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}


void OsxWrapper::setVideoMode(const VideoMode mode)
{
	_grabber->setVideoMode(mode);
}
