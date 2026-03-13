#include <grabber/directx/DirectXWrapper.h>

DirectXWrapper::DirectXWrapper( int updateRate_Hz,
								int display,
								int pixelDecimation,
								int cropLeft, int cropRight, int cropTop, int cropBottom
								)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(display, cropLeft, cropRight, cropTop, cropBottom)

{
	_grabber.setPixelDecimation(pixelDecimation);
}

DirectXWrapper::DirectXWrapper(const QJsonDocument& grabberConfig)
	: DirectXWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
					 0,
					 GrabberWrapper::DEFAULT_PIXELDECIMATION,
					 0,0,0,0)
{
	GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void DirectXWrapper::action()
{
	transferFrame(_grabber);
}
