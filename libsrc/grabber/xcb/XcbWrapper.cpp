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
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

XcbWrapper::~XcbWrapper()
{
	if ( _init )
	{
		stop();
	}
}

void XcbWrapper::action()
{
	if (! _init )
	{
		_init = true;
		if ( ! _grabber.setupDisplay() )
		{
			stop();
		}
		else
		{
			if (_grabber.updateScreenDimensions() < 0 )
			{
				stop();
			}
		}
	}

	if (isActive())
	{
		transferFrame(_grabber);
	}
}
