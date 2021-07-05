#include <grabber/DirectXWrapper.h>

DirectXWrapper::DirectXWrapper( int updateRate_Hz,
								int display,
								int pixelDecimation,
								int cropLeft, int cropRight, int cropTop, int cropBottom
								)
	: GrabberWrapper("DirectX", &_grabber, updateRate_Hz)
	  , _grabber(display, cropLeft, cropRight, cropTop, cropBottom)

{
	_grabber.setPixelDecimation(pixelDecimation);
}

void DirectXWrapper::action()
{
	transferFrame(_grabber);
}
