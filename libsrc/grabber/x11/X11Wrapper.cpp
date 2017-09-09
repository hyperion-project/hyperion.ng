#include <grabber/X11Wrapper.h>

X11Wrapper::X11Wrapper(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("X11", &_grabber, 0, 0, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(useXGetImage, cropLeft, cropRight, cropTop, cropBottom, horizontalPixelDecimation, verticalPixelDecimation)
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
