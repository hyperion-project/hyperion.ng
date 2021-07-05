#include <grabber/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(int pixelDecimation,	int updateRate_Hz)
	: GrabberWrapper("AmLogic", &_grabber, updateRate_Hz)
	  , _grabber()
{
	_grabber.setPixelDecimation(pixelDecimation);
}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
