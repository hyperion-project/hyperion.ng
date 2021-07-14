
#include "AmlogicWrapper.h"

// Linux includes
#include <unistd.h>

AmlogicWrapper::AmlogicWrapper( int updateRate_Hz,
								int pixelDecimation,
								int cropLeft, int cropRight,
								int cropTop, int cropBottom
								) :
	  _timer(this),
	  _grabber()
{
	_grabber.setFramerate(updateRate_Hz);
	_grabber.setCropping(cropLeft, cropRight, cropTop, cropBottom);
	_grabber.setPixelDecimation(pixelDecimation);

	_timer.setTimerType(Qt::PreciseTimer);
	_timer.setSingleShot(false);
	_timer.setInterval(_grabber.getUpdateInterval());

	// Connect capturing to the timeout signal of the timer
	connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & AmlogicWrapper::getScreenshot()
{
	capture();
	return _screenshot;
}

void AmlogicWrapper::start()
{
	_timer.start();
}

void AmlogicWrapper::stop()
{
	_timer.stop();
}

bool AmlogicWrapper::screenInit()
{
	return _grabber.setupScreen();
}

void AmlogicWrapper::capture()
{
	_grabber.grabFrame(_screenshot);

	emit sig_screenshot(_screenshot);
}

void AmlogicWrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
