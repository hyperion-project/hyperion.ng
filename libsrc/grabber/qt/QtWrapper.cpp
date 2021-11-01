#include <grabber/QtWrapper.h>

QtWrapper::QtWrapper( int updateRate_Hz,
					  int display,
					  int pixelDecimation,
					  int cropLeft, int cropRight, int cropTop, int cropBottom
					  )
	: GrabberWrapper("Qt", &_grabber, updateRate_Hz)
	  , _grabber(display, cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

bool QtWrapper::open()
{
	return _grabber.open();
}

void QtWrapper::action()
{
	transferFrame(_grabber);
}
