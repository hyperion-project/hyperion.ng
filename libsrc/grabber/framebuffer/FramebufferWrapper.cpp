#include <grabber/FramebufferWrapper.h>

FramebufferWrapper::FramebufferWrapper(const QString & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("FrameBuffer", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(device, grabWidth, grabHeight)
{
	_ggrabber = &_grabber;
}

FramebufferWrapper::~FramebufferWrapper()
{
}

void FramebufferWrapper::action()
{
	updateOutputSize();
	_grabber.grabFrame(_image);
	setImage();
}
