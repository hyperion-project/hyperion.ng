#include <grabber/amlogic/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(int updateRate_Hz,
							   int deviceIdx,
							   int pixelDecimation)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(deviceIdx)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

AmlogicWrapper::AmlogicWrapper(const QJsonDocument &grabberConfig)
	: AmlogicWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
					 grabberConfig["input"].toInt(0),
					 GrabberWrapper::DEFAULT_PIXELDECIMATION)
{
	handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
