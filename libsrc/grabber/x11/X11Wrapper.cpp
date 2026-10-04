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
	if (_grabber.isAvailable(true))
	{
		GrabberWrapper::handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
	}
}

X11Wrapper::~X11Wrapper()
{
	if ( _init )
	{
		GrabberWrapper::stop();
	}
}

bool X11Wrapper::start()
{
	if (_grabber.isAvailable())
	{
		qCDebug(grabber_screen_flow) << "Start grabber" << _grabber.getGrabberName() << "currently:" << (_grabber.isEnabled() ? "enabled" : "disabled");
		return GrabberWrapper::start();
	}

	return false;
}

void X11Wrapper::stop()
{
	qCDebug(grabber_screen_flow) << "Stop grabber" << _grabber.getGrabberName() << "currently:" << (_grabber.isEnabled() ? "enabled" : "disabled");
	if (_grabber.isAvailable())
	{
		GrabberWrapper::stop();
	}
}

bool X11Wrapper::open()
{
	return _grabber.setupDisplay();
}

bool X11Wrapper::close()
{
	return _grabber.close();
}

void X11Wrapper::action()
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
