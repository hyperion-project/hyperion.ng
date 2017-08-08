
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
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
	, _init(false)
	, _x11SetupSuccess(false)
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

	int result = _grabber->updateScreenDimensions();
	if (result >= 0 )
	{
		unsigned w = _grabber->getImageWidth();
		unsigned h = _grabber->getImageHeight();

		if ( result > 0 || _image.width() != w || _image.height() != h)
		{
			_processor->setSize(w, h);
			_image.resize(w, h);
		}
		// Grab frame into the allocated image
		_grabber->grabFrame(_image);

		emit emitImage(_priority, _image, _timeout_ms);
		_processor->process(_image, _ledColors);
		setColors(_ledColors, _timeout_ms);
	}
}
