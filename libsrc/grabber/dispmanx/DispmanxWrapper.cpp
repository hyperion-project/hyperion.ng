#include <grabber/dispmanx/DispmanxWrapper.h>

DispmanxWrapper::DispmanxWrapper( int updateRate_Hz,
								  int pixelDecimation
								  )
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
{
	if (_isAvailable)
	{
		_grabber.setPixelDecimation(pixelDecimation);
	}
}

DispmanxWrapper::DispmanxWrapper(const QJsonDocument& grabberConfig)
	: DispmanxWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
					  GrabberWrapper::DEFAULT_PIXELDECIMATION)
{
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

bool DispmanxWrapper::open()
{
	return _grabber.open();
}

void DispmanxWrapper::action()
{
	transferFrame(_grabber);
}
