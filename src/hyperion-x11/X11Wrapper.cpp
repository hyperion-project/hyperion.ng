
// Hyperion-X11 includes
#include "X11Wrapper.h"

X11Wrapper::X11Wrapper(int grabInterval, bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation) :
	_timer(this),
	_grabber(useXGetImage, cropLeft, cropRight, cropTop, cropBottom, horizontalPixelDecimation, verticalPixelDecimation)
{
	_timer.setSingleShot(false);
	_timer.setInterval(grabInterval);

	// Connect capturing to the timeout signal of the timer
	connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & X11Wrapper::getScreenshot()
{
	_grabber.grabFrame(_screenshot, true);
	return _screenshot;
}

void X11Wrapper::start()
{
	_timer.start();
}

void X11Wrapper::stop()
{
	_timer.stop();
}

bool X11Wrapper::displayInit()
{
	return _grabber.Setup();
}

void X11Wrapper::capture()
{
	_grabber.grabFrame(_screenshot, true);
	emit sig_screenshot(_screenshot);
}

void X11Wrapper::setVideoMode(const VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
