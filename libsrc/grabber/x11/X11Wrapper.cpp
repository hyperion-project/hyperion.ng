// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// X11 grabber includes
#include <grabber/X11Wrapper.h>
#include <grabber/X11Grabber.h>

X11Wrapper::X11Wrapper(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation, const unsigned updateRate_Hz, const int priority)
	: _updateInterval_ms(1000/updateRate_Hz)
	, _timeout_ms(2 * _updateInterval_ms)
	, _priority(priority)
	, _timer()
//	, _image(grabWidth, grabHeight)
	, _grabber(new X11Grabber(useXGetImage, cropLeft, cropRight, cropTop, cropBottom, horizontalPixelDecimation, verticalPixelDecimation))
	, _processor(ImageProcessorFactory::getInstance().newImageProcessor())
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
	, _hyperion(Hyperion::getInstance())
	, _init(false)
	, _x11SetupSuccess(false)
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);
	_timer.setSingleShot(false);

	// Connect the QTimer to this
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(action()));
}

X11Wrapper::~X11Wrapper()
{
	// Cleanup used resources (ImageProcessor and FrameGrabber)
	delete _processor;
	delete _grabber;
}

void X11Wrapper::start()
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
		_timer.start();
		_hyperion->registerPriority("X11 Grabber", _priority);
	}

	ErrorIf( ! _x11SetupSuccess, Logger::getInstance("X11"), "X11 Grabber start failed");
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
	_hyperion->setColors(_priority, _ledColors, _timeout_ms);
}
void X11Wrapper::stop()
{
	// Stop the timer, effectivly stopping the process
	_timer.stop();
	_hyperion->unRegisterPriority("X11 Grabber");
}

void X11Wrapper::setGrabbingMode(const GrabbingMode mode)
{
	switch (mode)
	{
	case GRABBINGMODE_VIDEO:
	case GRABBINGMODE_PAUSE:
	case GRABBINGMODE_AUDIO:
	case GRABBINGMODE_PHOTO:
	case GRABBINGMODE_MENU:
	case GRABBINGMODE_SCREENSAVER:
	case GRABBINGMODE_INVALID:
		start();
		break;
	case GRABBINGMODE_OFF:
		stop();
		break;
	}
}

void X11Wrapper::setVideoMode(const VideoMode mode)
{
	_grabber->setVideoMode(mode);
}
