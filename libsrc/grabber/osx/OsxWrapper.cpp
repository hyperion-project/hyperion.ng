#include <grabber/OsxWrapper.h>

OsxWrapper::OsxWrapper( int updateRate_Hz,
						int display,
						int pixelDecimation
						)
	: GrabberWrapper("OSX", &_grabber, updateRate_Hz)
	  , _grabber(display)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

void OsxWrapper::action()
{
	transferFrame(_grabber);
}
