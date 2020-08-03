
// Hyperion-Xcb includes
#include "XcbWrapper.h"

XcbWrapper::XcbWrapper(int grabInterval, int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation) :
	_timer(this),
	_grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation)
{
	_timer.setSingleShot(false);
	_timer.setInterval(grabInterval);

	// Connect capturing to the timeout signal of the timer
	connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & XcbWrapper::getScreenshot()
{
	_grabber.grabFrame(_screenshot, true);
	return _screenshot;
}

void XcbWrapper::start()
{
	_timer.start();
}

void XcbWrapper::stop()
{
	_timer.stop();
}

bool XcbWrapper::displayInit()
{
	return _grabber.Setup();
}

void XcbWrapper::capture()
{
	_grabber.grabFrame(_screenshot, !_inited);
	emit sig_screenshot(_screenshot);
	_inited = true;
}

void XcbWrapper::setVideoMode(const VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
