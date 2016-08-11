// QT includes
#include <QDateTime>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Dispmanx grabber includes
#include <grabber/DispmanxWrapper.h>
#include <grabber/DispmanxFrameGrabber.h>


DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("Dispmanx", priority)
	, _updateInterval_ms(1000/updateRate_Hz)
	, _timeout_ms(2 * _updateInterval_ms)
	, _image(grabWidth, grabHeight)
	, _grabber(new DispmanxFrameGrabber(grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);

	_processor->setSize(grabWidth, grabHeight);
}

DispmanxWrapper::~DispmanxWrapper()
{
	delete _grabber;
}

void DispmanxWrapper::action()
{
	// Grab frame into the allocated image
	_grabber->grabFrame(_image);

	if ( _forward )
	{
		Image<ColorRgb> image_rgb;
		_image.toRgb(image_rgb);
		emit emitImage(_priority, image_rgb, _timeout_ms);
	}

	_processor->process(_image, _ledColors);
	_hyperion->setColors(_priority, _ledColors, _timeout_ms);
}

void DispmanxWrapper::setGrabbingMode(const GrabbingMode mode)
{
	switch (mode)
	{
	case GRABBINGMODE_VIDEO:
	case GRABBINGMODE_PAUSE:
		_grabber->setFlags(DISPMANX_SNAPSHOT_NO_RGB|DISPMANX_SNAPSHOT_FILL);
		start();
		break;
	case GRABBINGMODE_AUDIO:
	case GRABBINGMODE_PHOTO:
	case GRABBINGMODE_MENU:
	case GRABBINGMODE_SCREENSAVER:
	case GRABBINGMODE_INVALID:
		_grabber->setFlags(0);
		start();
		break;
	case GRABBINGMODE_OFF:
		stop();
		break;
	}
}

void DispmanxWrapper::setVideoMode(const VideoMode mode)
{
	_grabber->setVideoMode(mode);
}

void DispmanxWrapper::setCropping(const unsigned cropLeft, const unsigned cropRight,
	const unsigned cropTop, const unsigned cropBottom)
{
	_grabber->setCropping(cropLeft, cropRight, cropTop, cropBottom);
}
