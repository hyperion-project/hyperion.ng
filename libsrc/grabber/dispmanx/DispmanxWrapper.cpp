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
	: GrabberWrapper("Dispmanx", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _image_rgba(grabWidth, grabHeight)
	, _grabber(new DispmanxFrameGrabber(grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	_ggrabber = _grabber;
}

DispmanxWrapper::~DispmanxWrapper()
{
	delete _grabber;
}

void DispmanxWrapper::action()
{
	_grabber->grabFrame(_image_rgba);
	_image_rgba.toRgb(_image);
	setImage();
}

void DispmanxWrapper::setCropping(const unsigned cropLeft, const unsigned cropRight,
	const unsigned cropTop, const unsigned cropBottom)
{
	_grabber->setCropping(cropLeft, cropRight, cropTop, cropBottom);
}
