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
	: GrabberWrapper("AmLogic", priority)
	, _updateInterval_ms(1000/updateRate_Hz)
	, _timeout_ms(2 * _updateInterval_ms)
	, _image(grabWidth, grabHeight)
	, _grabber(new AmlogicGrabber(grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);

	_processor->setSize(grabWidth, grabHeight);
}

AmlogicWrapper::~AmlogicWrapper()
{
	delete _grabber;
}

void AmlogicWrapper::action()
{
	// Grab frame into the allocated image
	if (_grabber->grabFrame(_image) < 0)
	{
		// Frame grab failed, maybe nothing playing or ....
		return;
	}

	Image<ColorRgb> image_rgb;
	_image.toRgb(image_rgb);
	emit emitImage(_priority, image_rgb, _timeout_ms);

	_processor->process(_image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}

void AmlogicWrapper::setVideoMode(const VideoMode mode)
{
	_grabber->setVideoMode(mode);
}
