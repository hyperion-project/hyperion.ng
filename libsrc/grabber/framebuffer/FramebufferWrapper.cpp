#include <grabber/FramebufferWrapper.h>

FramebufferWrapper::FramebufferWrapper(const QString & device, unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz)
	: GrabberWrapper("FrameBuffer", &_grabber, grabWidth, grabHeight, updateRate_Hz)
	, _grabber(device, grabWidth, grabHeight)
{}

void FramebufferWrapper::action()
{
	transferFrame(_grabber);
}
