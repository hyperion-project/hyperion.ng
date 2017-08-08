
// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// X11 grabber includes
#include <grabber/X11Wrapper.h>
#include <grabber/X11Grabber.h>

X11Wrapper::X11Wrapper(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("X11", 0, 0, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(new X11Grabber(useXGetImage, cropLeft, cropRight, cropTop, cropBottom, horizontalPixelDecimation, verticalPixelDecimation))
	, _init(false)
{
	_ggrabber = _grabber;
}

X11Wrapper::~X11Wrapper()
{
	delete _grabber;
}

void X11Wrapper::action()
{
	if (! _init )
	{
		_init = true;
		if ( ! _grabber->Setup() )
		{
			stop();
		}
	}

	if (_grabber->updateScreenDimensions() >= 0 )
	{
		updateOutputSize();
		_grabber->grabFrame(_image);
		setImage();
	}
}
