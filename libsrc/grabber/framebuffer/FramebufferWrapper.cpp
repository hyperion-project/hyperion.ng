// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Framebuffer grabber includes
#include <grabber/FramebufferWrapper.h>
#include <grabber/FramebufferFrameGrabber.h>

FramebufferWrapper::FramebufferWrapper(const std::string & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("FrameBuffer", priority)
	, _updateInterval_ms(1000/updateRate_Hz)
	, _timeout_ms(2 * _updateInterval_ms)
	, _image(grabWidth, grabHeight)
	, _grabber(new FramebufferFrameGrabber(device, grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);

	_processor->setSize(grabWidth, grabHeight);
}

FramebufferWrapper::~FramebufferWrapper()
{
	delete _grabber;
}

void FramebufferWrapper::action()
{
	// Grab frame into the allocated image
	_grabber->grabFrame(_image);

	emit emitImage(_priority, _image, _timeout_ms);
	
	_processor->process(_image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}

void FramebufferWrapper::setVideoMode(const VideoMode mode)
{
	_grabber->setVideoMode(mode);
}
