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

	Image<ColorRgb> image_rgb;
	_image.toRgb(image_rgb);
	emit emitImage(_priority, image_rgb, _timeout_ms);

	_processor->process(_image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}

void DispmanxWrapper::kodiPlay()
{
	_grabber->setFlags(DISPMANX_SNAPSHOT_NO_RGB|DISPMANX_SNAPSHOT_FILL);
	GrabberWrapper::kodiPlay();
	
}

void DispmanxWrapper::kodiPause()
{
	_grabber->setFlags(0);
	GrabberWrapper::kodiPause();
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
