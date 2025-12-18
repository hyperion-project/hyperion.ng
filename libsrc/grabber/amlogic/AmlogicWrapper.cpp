#include <grabber/amlogic/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(int updateRate_Hz,
							   int deviceIdx,
							   int pixelDecimation,
							   int cropLeft, int cropRight, int cropTop, int cropBottom
							)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(deviceIdx, cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

AmlogicWrapper::AmlogicWrapper(const QJsonDocument &grabberConfig)
	: AmlogicWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
					 grabberConfig["input"].toInt(0),
				GrabberWrapper::DEFAULT_PIXELDECIMATION,
				0,0,0,0)
{
	handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
