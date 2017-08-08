// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Osx grabber includes
#include <grabber/OsxWrapper.h>
#include <grabber/OsxFrameGrabber.h>

OsxWrapper::OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("OSX FrameGrabber", updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(new OsxFrameGrabber(display, grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	// Configure the timer to generate events every n milliseconds
	_ggrabber = _grabber;
	_processor->setSize(grabWidth, grabHeight);
	_image.resize(grabWidth, grabHeight);
}

OsxWrapper::~OsxWrapper()
{
	delete _grabber;
}

void OsxWrapper::action()
{
	int w = _grabber->getImageWidth();
	int h = _grabber->getImageHeight();

	if ( result > 0 || _image.width() != w || _image.height() != h)
	{
		_processor->setSize(w, h);
		_image.resize(w, h);
	}

	// Grab frame into the allocated image
	_grabber->grabFrame(_image);

	emit emitImage(_priority, _image, _timeout_ms);

	_processor->process(_image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}
