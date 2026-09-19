#include <grabber/gamescope/GamescopeWrapper.h>

GamescopeWrapper::GamescopeWrapper(int updateRate_Hz,
	int pixelDecimation,
	int cropLeft, int cropRight, int cropTop, int cropBottom)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

GamescopeWrapper::GamescopeWrapper(const QJsonDocument& grabberConfig)
	: GamescopeWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
		GrabberWrapper::DEFAULT_PIXELDECIMATION,
		0, 0, 0, 0)
{
	if (_grabber.isAvailable())
	{
		GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

void GamescopeWrapper::action()
{
	if (!_grabber.isAvailable())
	{
		return;
	}

	transferFrame(_grabber);
}
