#include <grabber/drm/DRMWrapper.h>

DRMWrapper::DRMWrapper(int updateRate_Hz,
					   int deviceIdx,
					   int pixelDecimation,
					   int cropLeft, int cropRight, int cropTop, int cropBottom)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(deviceIdx, cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

DRMWrapper::DRMWrapper(const QJsonDocument &grabberConfig)
	: DRMWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
				 grabberConfig["input"].toInt(0),
				 GrabberWrapper::DEFAULT_PIXELDECIMATION,
				 0, 0, 0, 0)
{
	_isAvailable = _grabber.isAvailable();
	if (_isAvailable)
	{
		handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

void DRMWrapper::action()
{
	if (!_isAvailable)
	{
		return;
	}

	transferFrame(_grabber);
}
