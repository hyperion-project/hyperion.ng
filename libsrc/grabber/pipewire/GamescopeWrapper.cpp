#include <grabber/pipewire/GamescopeWrapper.h>

GamescopeWrapper::GamescopeWrapper(int updateRate_Hz,
                                   int pixelDecimation,
                                   int cropLeft, int cropRight,
                                   int cropTop,  int cropBottom)
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
	if (_grabber.isAvailable(true))
	{
		GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

GamescopeWrapper::~GamescopeWrapper()
{
	_grabber.close();
}

bool GamescopeWrapper::start()
{
	if (_grabber.isAvailable() && _grabber.open())
	{
		return GrabberWrapper::start();
	}
	return false;
}

void GamescopeWrapper::stop()
{
	GrabberWrapper::stop();
	_grabber.close();
}

void GamescopeWrapper::action()
{
	transferFrame(_grabber);
}

