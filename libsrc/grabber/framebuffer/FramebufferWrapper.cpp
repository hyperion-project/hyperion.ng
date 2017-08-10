#include <grabber/FramebufferWrapper.h>

FramebufferWrapper::FramebufferWrapper(const QString & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("FrameBuffer", &_grabber, grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(device, grabWidth, grabHeight)
{}

void FramebufferWrapper::action()
{
	transferFrame(_grabber);
}
