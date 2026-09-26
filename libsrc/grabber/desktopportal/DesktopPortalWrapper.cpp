#include <grabber/desktopportal/DesktopPortalWrapper.h>

DesktopPortalWrapper::DesktopPortalWrapper(int updateRate_Hz,
	int pixelDecimation,
	int cropLeft, int cropRight, int cropTop, int cropBottom)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

DesktopPortalWrapper::DesktopPortalWrapper(const QJsonDocument& grabberConfig)
	: DesktopPortalWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
		GrabberWrapper::DEFAULT_PIXELDECIMATION,
		0, 0, 0, 0)
{
	if (_grabber.isAvailable())
	{
		GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

void DesktopPortalWrapper::action()
{
	if (!_grabber.isAvailable())
	{
		return;
	}

	transferFrame(_grabber);
}
