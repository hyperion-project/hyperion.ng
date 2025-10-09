#include <grabber/x11/X11Wrapper.h>

X11Wrapper::X11Wrapper( int updateRate_Hz,
						int pixelDecimation,
						int cropLeft, int cropRight, int cropTop, int cropBottom)
	: GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom)
	, _init(false)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

X11Wrapper::X11Wrapper(const QJsonDocument& grabberConfig)
	: X11Wrapper(GrabberWrapper::DEFAULT_RATE_HZ,
				 GrabberWrapper::DEFAULT_PIXELDECIMATION,
				 0,0,0,0)
{
	_isAvailable = _grabber.isAvailable();
	if (_isAvailable)
	{
		this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

X11Wrapper::~X11Wrapper()
{
	if ( _init )
	{
		stop();
	}
}


bool X11Wrapper::isAvailable()
{
	return _isAvailable;
}

bool X11Wrapper::start()
{
	if (_isAvailable)
	{
		return GrabberWrapper::start();
	}

	return false;
}


bool X11Wrapper::open()
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

void X11Wrapper::action()
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
