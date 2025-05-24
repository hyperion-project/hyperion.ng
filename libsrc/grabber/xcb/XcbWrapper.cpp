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
	_isAvailable = _grabber.isAvailable();
	if (_isAvailable)
	{
		this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

XcbWrapper::~XcbWrapper()
{
	if ( _init )
	{
		stop();
	}
}

bool XcbWrapper::isAvailable()
{
	return _isAvailable;
}

bool XcbWrapper::start()
{
	if (_isAvailable)
	{
		return GrabberWrapper::start();
	}

	return false;
}


bool XcbWrapper::open()
{
	bool isOpen {false};
	if ( _isAvailable && _grabber.setupDisplay())
	{
		if (_grabber.updateScreenDimensions() >= 0)
		{
			isOpen = true;
		}
	}

	return isOpen;
}

void XcbWrapper::action()
{
	if (!_isAvailable)
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
