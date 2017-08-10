#include <grabber/OsxWrapper.h>

OsxWrapper::OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("OSX FrameGrabber", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(display, grabWidth, grabHeight)
{
	_ggrabber = &_grabber;
}

void OsxWrapper::action()
{
	transferFrame(_grabber);
}
