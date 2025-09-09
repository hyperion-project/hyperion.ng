#include <grabber/xcb/XcbWrapper.h>

XcbWrapper::XcbWrapper( int updateRate_Hz,
						int pixelDecimation,
						int cropLeft, int cropRight, int cropTop, int cropBottom)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom)
	, _init(false)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

XcbWrapper::XcbWrapper(const QJsonDocument& grabberConfig)
	: XcbWrapper(GrabberWrapper::DEFAULT_RATE_HZ,
				 GrabberWrapper::DEFAULT_PIXELDECIMATION,
				 0,0,0,0)
{
	if (_grabber.isAvailable(true))
	{
		GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

XcbWrapper::~XcbWrapper()
{
	if ( _init )
	{
		GrabberWrapper::stop();
	}
}

bool XcbWrapper::start()
{
	if (_grabber.isAvailable())
	{
		return GrabberWrapper::start();
	}

	return false;
}


bool XcbWrapper::open()
{
	return _grabber.setupDisplay();
}

void XcbWrapper::action()
{
	if (!_grabber.isAvailable())
	{
		return;
	}

	if (! _init )
	{
		_init = true;
		if ( ! open() )
		{
			stop();
		}
	}

	if (isActive())
	{
		transferFrame(_grabber);
	}
}
