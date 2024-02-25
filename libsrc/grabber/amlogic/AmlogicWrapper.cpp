#include <grabber/amlogic/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(int pixelDecimation,	int updateRate_Hz)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	  , _grabber()
{
	_grabber.setPixelDecimation(pixelDecimation);
}

AmlogicWrapper::AmlogicWrapper(const QJsonDocument& grabberConfig)
	: GrabberWrapper(GRABBERTYPE, &_grabber)
{
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
