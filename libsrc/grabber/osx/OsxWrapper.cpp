#include <grabber/OsxWrapper.h>

OsxWrapper::OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("OSX FrameGrabber", &_grabber, grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(display, grabWidth, grabHeight)
{}

void OsxWrapper::action()
{
	transferFrame(_grabber);
}
