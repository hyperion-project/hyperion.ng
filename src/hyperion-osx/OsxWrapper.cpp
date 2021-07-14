
#include "OsxWrapper.h"

OsxWrapper::OsxWrapper( int updateRate_Hz,
						int display,
						int pixelDecimation,
						int cropLeft, int cropRight,
						int cropTop, int cropBottom
						) :
	  _timer(this),
	  _grabber(display)
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

const Image<ColorRgb> & OsxWrapper::getScreenshot()
{
	capture();
	return _screenshot;
}

bool OsxWrapper::displayInit()
{
	return _grabber.setupDisplay();
}

void OsxWrapper::start()
{
	_timer.start();
}

void OsxWrapper::stop()
{
	_timer.stop();
}

void OsxWrapper::capture()
{
	_grabber.grabFrame(_screenshot);
	emit sig_screenshot(_screenshot);
}

void OsxWrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}

