#include <grabber/pipewire/WaylandWrapper.h>

const int WaylandWrapper::MAX_GRAB_RATE_HZ = 240;

WaylandWrapper::WaylandWrapper(int updateRate_Hz,
							   int pixelDecimation,
							   int cropLeft, int cropRight, int cropTop, int cropBottom)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

WaylandWrapper::WaylandWrapper(const QJsonDocument& grabberConfig)
	: WaylandWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
					 GrabberWrapper::DEFAULT_PIXELDECIMATION,
					 0, 0, 0, 0)
{
	if (_grabber.isAvailable(true))
	{
		GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

WaylandWrapper::~WaylandWrapper()
{
	_grabber.close();
}

bool WaylandWrapper::start()
{
	// Do not open the portal (sharing dialog/restored session) if capture is disabled
	if (_grabber.isAvailable() && _grabber.isEnabled() && _grabber.open())
	{
		return GrabberWrapper::start();
	}

	return false;
}

void WaylandWrapper::stop()
{
	GrabberWrapper::stop();
	_grabber.close();
}

void WaylandWrapper::action()
{
	transferFrame(_grabber);
}
