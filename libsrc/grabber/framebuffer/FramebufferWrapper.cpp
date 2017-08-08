// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Framebuffer grabber includes
#include <grabber/FramebufferWrapper.h>
#include <grabber/FramebufferFrameGrabber.h>

FramebufferWrapper::FramebufferWrapper(const QString & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("FrameBuffer", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(new FramebufferFrameGrabber(device, grabWidth, grabHeight))
{
	_ggrabber = _grabber;
}

FramebufferWrapper::~FramebufferWrapper()
{
	delete _grabber;
}

void FramebufferWrapper::action()
{
	updateOutputSize();
	_grabber->grabFrame(_image);
	setImage();
}
