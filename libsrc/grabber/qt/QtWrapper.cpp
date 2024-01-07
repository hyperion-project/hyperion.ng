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
	: GrabberWrapper(GRABBERTYPE, &_grabber)
{
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

bool QtWrapper::open()
{
	return _grabber.open();
}

void QtWrapper::action()
{
	transferFrame(_grabber);
}
