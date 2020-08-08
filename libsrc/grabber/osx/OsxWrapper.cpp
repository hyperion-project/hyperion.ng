#include <grabber/OsxWrapper.h>

OsxWrapper::OsxWrapper(unsigned display, unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz)
	: GrabberWrapper("OSX FrameGrabber", &_grabber, grabWidth, grabHeight, updateRate_Hz)
	, _grabber(display, grabWidth, grabHeight)
{}

void OsxWrapper::action()
{
	transferFrame(_grabber);
}
