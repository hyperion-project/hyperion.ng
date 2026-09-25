
#include "DispmanxWrapper.h"

DispmanxWrapper::DispmanxWrapper( int updateRate_Hz,
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

const Image<ColorRgb> & DispmanxWrapper::getScreenshot()
{
	capture();
	return _screenshot;
}

bool DispmanxWrapper::open()
{
	return _grabber.open();
}

void DispmanxWrapper::start()
{
	_timer.start();
}

void DispmanxWrapper::stop()
{
	_timer.stop();
}

bool DispmanxWrapper::screenInit()
{
	return (open() && _grabber.setupScreen());
}

void DispmanxWrapper::capture()
{
	if ( open() )
	{
		_grabber.grabFrame(_screenshot);
		emit sig_screenshot(_screenshot);
	}
}

void DispmanxWrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}

