#include <grabber/FramebufferWrapper.h>

FramebufferWrapper::FramebufferWrapper( int updateRate_Hz,
										const QString & device,
										int pixelDecimation)
	: GrabberWrapper("FrameBuffer", &_grabber, updateRate_Hz)
	  , _grabber(device)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

void FramebufferWrapper::action()
{
	transferFrame(_grabber);
}
