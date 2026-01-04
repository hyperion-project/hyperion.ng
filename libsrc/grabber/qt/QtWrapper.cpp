#include <grabber/qt/QtWrapper.h>

QtWrapper::QtWrapper( int updateRate_Hz,
					  int display,
					  int pixelDecimation,
					  int cropLeft, int cropRight, int cropTop, int cropBottom
					  )
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(display, cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

QtWrapper::QtWrapper(const QJsonDocument& grabberConfig)
	: QtWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
				grabberConfig["input"].toInt(0),
				GrabberWrapper::DEFAULT_PIXELDECIMATION,
				0,0,0,0)
{
	if (_grabber.isAvailable(true))
	{
		GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

bool QtWrapper::start()
{
	if (_grabber.isAvailable())
	{
		return GrabberWrapper::start();
	}

	return false;
}

void QtWrapper::action()
{
	if (!_grabber.isAvailable())
	{
		return;
	}

	transferFrame(_grabber);
}
