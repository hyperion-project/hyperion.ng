#include <grabber/DispmanxWrapper.h>

DispmanxWrapper::DispmanxWrapper( int updateRate_Hz,
								  int pixelDecimation
								  )
	: GrabberWrapper("Dispmanx", &_grabber, updateRate_Hz)
	  , _grabber()
{
	_grabber.setPixelDecimation(pixelDecimation);
}

bool DispmanxWrapper::open()
{
	return _grabber.open();
}

void DispmanxWrapper::action()
{
	transferFrame(_grabber);
}
