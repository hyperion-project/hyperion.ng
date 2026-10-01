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
	: OsxWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
				 kCGDirectMainDisplay,
				 GrabberWrapper::DEFAULT_PIXELDECIMATION)
{
	GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

bool OsxWrapper::start()
{
	const bool rc = GrabberWrapper::start();
	if (rc)
	{
		// The continuous capture session is only required while the grabber is active
		_grabber.startStream();
	}
	return rc;
}

void OsxWrapper::stop()
{
	_grabber.stopStream();
	GrabberWrapper::stop();
}

void OsxWrapper::action()
{
	transferFrame(_grabber);
}
