
// Hyperion-Dispmanx includes
#include "DispmanxWrapper.h"

DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight,
	const VideoMode& videoMode,
	const unsigned cropLeft, const unsigned cropRight,
	const unsigned cropTop, const unsigned cropBottom,
	const unsigned updateRate_Hz) :
	_timer(this),
	_grabber(grabWidth, grabHeight)
{
	_grabber.setVideoMode(videoMode);
	_grabber.setCropping(cropLeft, cropRight, cropTop, cropBottom);
	_timer.setSingleShot(false);
	_timer.setInterval(updateRate_Hz);

	// Connect capturing to the timeout signal of the timer
	connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & DispmanxWrapper::getScreenshot()
{
	capture();
	return _screenshot;
}

void DispmanxWrapper::start()
{
	_timer.start();
}

void DispmanxWrapper::stop()
{
	_timer.stop();
}

void DispmanxWrapper::capture()
{
	_grabber.grabFrame(_screenshot);
	emit sig_screenshot(_screenshot);
}
