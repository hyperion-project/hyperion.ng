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
	: FramebufferWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
						 0,
						 GrabberWrapper::DEFAULT_PIXELDECIMATION)
{
	GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void FramebufferWrapper::action()
{
	transferFrame(_grabber);
}
