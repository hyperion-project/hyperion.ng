#include <grabber/QtWrapper.h>

QtWrapper::QtWrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display, unsigned updateRate_Hz)
	: GrabberWrapper("Qt", &_grabber, 0, 0, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation, display)
{}

bool QtWrapper::open()
{
	return _grabber.open();
}

void QtWrapper::action()
{
	transferFrame(_grabber);
}
