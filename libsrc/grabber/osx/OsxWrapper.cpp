#include <grabber/osx/OsxWrapper.h>

OsxWrapper::OsxWrapper( int updateRate_Hz,
						int display,
						int pixelDecimation
						)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	  , _grabber(display)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

OsxWrapper::OsxWrapper(const QJsonDocument& grabberConfig)
	: GrabberWrapper(GRABBERTYPE, &_grabber)
{
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void OsxWrapper::action()
{
	transferFrame(_grabber);
}
