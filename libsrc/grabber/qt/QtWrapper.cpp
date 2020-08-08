#include <grabber/QtWrapper.h>

QtWrapper::QtWrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display, unsigned updateRate_Hz)
	: GrabberWrapper("Qt", &_grabber, 0, 0, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation, display)
{}

void QtWrapper::action()
{
	transferFrame(_grabber);
}
