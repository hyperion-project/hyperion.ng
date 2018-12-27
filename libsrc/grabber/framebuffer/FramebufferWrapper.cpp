#include <grabber/FramebufferWrapper.h>

FramebufferWrapper::FramebufferWrapper(const QString & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz)
	: GrabberWrapper("FrameBuffer", &_grabber, grabWidth, grabHeight, updateRate_Hz)
	, _grabber(device, grabWidth, grabHeight)
{}

void FramebufferWrapper::action()
{
	transferFrame(_grabber);
}
