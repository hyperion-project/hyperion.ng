#include <grabber/drm/DRMWrapper.h>

DRMWrapper::DRMWrapper( int updateRate_Hz,
						const QString & device,
						int pixelDecimation,
						int cropLeft, int cropRight, int cropTop, int cropBottom
						)
	: GrabberWrapper("DRM", &_grabber, updateRate_Hz)
	  , _grabber(device, cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

DRMWrapper::DRMWrapper(const QJsonDocument& grabberConfig)
	: DRMWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
				"/dev/dri/card0",
				GrabberWrapper::DEFAULT_PIXELDECIMATION,
				0,0,0,0)
{
	_isAvailable = _grabber.isAvailable();
	if (_isAvailable)
	{
		this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}


void DRMWrapper::action()
{
	transferFrame(_grabber);
}
