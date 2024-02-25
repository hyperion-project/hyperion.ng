#include <grabber/framebuffer/FramebufferWrapper.h>

FramebufferWrapper::FramebufferWrapper( int updateRate_Hz,
										int deviceIdx,
										int pixelDecimation)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	  , _grabber(deviceIdx)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

FramebufferWrapper::FramebufferWrapper(const QJsonDocument& grabberConfig)
	: GrabberWrapper(GRABBERTYPE, &_grabber)
{
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void FramebufferWrapper::action()
{
	transferFrame(_grabber);
}
