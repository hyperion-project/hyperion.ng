// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Framebuffer grabber includes
#include <grabber/FramebufferWrapper.h>
#include "FramebufferFrameGrabber.h"

FramebufferWrapper::FramebufferWrapper(const std::string & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, Hyperion * hyperion) :
	_updateInterval_ms(1000/updateRate_Hz),
	_timeout_ms(2 * _updateInterval_ms),
	_priority(1000),
	_timer(),
	_image(grabWidth, grabHeight),
	_frameGrabber(new FramebufferFrameGrabber(device, grabWidth, grabHeight)),
	_processor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_ledColors(hyperion->getLedCount(), ColorRgb{0,0,0}),
	_hyperion(hyperion)
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);
	_timer.setSingleShot(false);

	_processor->setSize(grabWidth, grabHeight);

	// Connect the QTimer to this
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(action()));
}

FramebufferWrapper::~FramebufferWrapper()
{
	// Cleanup used resources (ImageProcessor and FrameGrabber)
	delete _processor;
	delete _frameGrabber;
}

void FramebufferWrapper::start()
{
	// Start the timer with the pre configured interval
	_timer.start();
}

void FramebufferWrapper::action()
{
	// Grab frame into the allocated image
	_frameGrabber->grabFrame(_image);

	_processor->process(_image, _ledColors);

	_hyperion->setColors(_priority, _ledColors, _timeout_ms);
}
void FramebufferWrapper::stop()
{
	// Stop the timer, effectivly stopping the process
	_timer.stop();
}

void FramebufferWrapper::setGrabbingMode(const GrabbingMode mode)
{
	switch (mode)
	{
	case GRABBINGMODE_VIDEO:
	case GRABBINGMODE_AUDIO:
	case GRABBINGMODE_PHOTO:
	case GRABBINGMODE_MENU:
	case GRABBINGMODE_INVALID:
		start();
		break;
	case GRABBINGMODE_OFF:
		stop();
		break;
	}
}

void FramebufferWrapper::setVideoMode(const VideoMode mode)
{
	_frameGrabber->setVideoMode(mode);
}
