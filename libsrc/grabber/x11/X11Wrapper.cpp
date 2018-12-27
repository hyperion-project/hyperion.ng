#include <grabber/X11Wrapper.h>

X11Wrapper::X11Wrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, const unsigned updateRate_Hz)
	: GrabberWrapper("X11", &_grabber, 0, 0, updateRate_Hz)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation)
	, _init(false)
{}

void X11Wrapper::action()
{
	if (! _init )
	{
		_init = true;
		if ( ! _grabber.Setup() )
		{
			stop();
		}
	}

	if (_grabber.updateScreenDimensions() >= 0 )
	{
		transferFrame(_grabber);
	}
}
