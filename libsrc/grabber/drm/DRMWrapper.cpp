#include <grabber/DRMWrapper.h>

DRMWrapper::DRMWrapper( int updateRate_Hz, const QString & device, int pixelDecimation)
	: GrabberWrapper("DRM", &_grabber, updateRate_Hz)
	  , _grabber(device)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

void DRMWrapper::action()
{
	transferFrame(_grabber);
}
