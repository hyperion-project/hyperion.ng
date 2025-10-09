#include <grabber/amlogic/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(int updateRate_Hz, int pixelDecimation)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber()
{
	_grabber.setPixelDecimation(pixelDecimation);
}

AmlogicWrapper::AmlogicWrapper(const QJsonDocument& grabberConfig)
	: AmlogicWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
					 GrabberWrapper::DEFAULT_PIXELDECIMATION)
{
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
