#include <grabber/X11Wrapper.h>

X11Wrapper::X11Wrapper( int updateRate_Hz,
						int pixelDecimation,
						int cropLeft, int cropRight, int cropTop, int cropBottom)
	: GrabberWrapper("X11", &_grabber, updateRate_Hz)
	  , _grabber(cropLeft, cropRight, cropTop, cropBottom)
	  , _init(false)
{
	_grabber.setPixelDecimation(pixelDecimation);
}

X11Wrapper::~X11Wrapper()
{
	if ( _init )
	{
		stop();
	}
}

void X11Wrapper::action()
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
