#include <grabber/X11Wrapper.h>

X11Wrapper::X11Wrapper(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("X11", 0, 0, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(useXGetImage, cropLeft, cropRight, cropTop, cropBottom, horizontalPixelDecimation, verticalPixelDecimation)
	, _init(false)
{
	_ggrabber = &_grabber;
}

X11Wrapper::~X11Wrapper()
{
}

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
		updateOutputSize();
		if (_grabber.grabFrame(_image) >= 0) setImage();
	}
}
