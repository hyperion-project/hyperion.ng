#include <grabber/XcbWrapper.h>

XcbWrapper::XcbWrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, const unsigned updateRate_Hz)
	: GrabberWrapper("Xcb", &_grabber, 0, 0, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation)
	, _init(false)
{}

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
		if ( ! _grabber.Setup() )
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
