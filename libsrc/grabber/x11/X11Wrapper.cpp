// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// X11 grabber includes
#include <grabber/X11Wrapper.h>
#include <grabber/X11Grabber.h>

X11Wrapper::X11Wrapper(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("X11", priority)
	, _updateInterval_ms(1000/updateRate_Hz)
	, _timeout_ms(2 * _updateInterval_ms)
	, _grabber(new X11Grabber(useXGetImage, cropLeft, cropRight, cropTop, cropBottom, horizontalPixelDecimation, verticalPixelDecimation))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
	, _init(false)
	, _x11SetupSuccess(false)
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);
}

X11Wrapper::~X11Wrapper()
{
	delete _grabber;
}

bool X11Wrapper::start()
{
	if (! _init )
	{
		_init = true;
		_x11SetupSuccess = _grabber->Setup();
		if ( _x11SetupSuccess )
		{
			_x11SetupSuccess = (_grabber->updateScreenDimensions() >= 0);
			_processor->setSize(_grabber->getImageWidth(), _grabber->getImageHeight());
			_image.resize(_grabber->getImageWidth(), _grabber->getImageHeight());
		}
	}
	// Start the timer with the pre configured interval
	if ( _x11SetupSuccess )
	{
		GrabberWrapper::start();
	}

	ErrorIf( ! _x11SetupSuccess, _log, "X11 Grabber start failed");
	return _x11SetupSuccess;
}


void X11Wrapper::action()
{
	int result = _grabber->updateScreenDimensions();
	if (result < 0 )
	{
		return;
	}
	if ( result > 0 )
	{
		_processor->setSize(_grabber->getImageWidth(), _grabber->getImageHeight());
		_image.resize(_grabber->getImageWidth(), _grabber->getImageHeight());
	}
	// Grab frame into the allocated image
	_grabber->grabFrame(_image);

	emit emitImage(_priority, _image, _timeout_ms);

	_processor->process(_image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}


void X11Wrapper::setVideoMode(const VideoMode mode)
{
	_grabber->setVideoMode(mode);
}
