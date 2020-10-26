#include <grabber/DirectXWrapper.h>

DirectXWrapper::DirectXWrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display, const unsigned updateRate_Hz)
	: GrabberWrapper("DirectX", &_grabber, 0, 0, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation, display)
{}

void DirectXWrapper::action()
{
	transferFrame(_grabber);
}
