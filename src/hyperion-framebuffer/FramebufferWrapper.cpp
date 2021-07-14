
#include "FramebufferWrapper.h"

FramebufferWrapper::FramebufferWrapper( int updateRate_Hz,
										const QString & device,
										int pixelDecimation,
										int cropLeft, int cropRight,
										int cropTop, int cropBottom
										) :
	  _timer(this),
	  _grabber(device)
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

const Image<ColorRgb> & FramebufferWrapper::getScreenshot()
{
	capture();
	return _screenshot;
}

void FramebufferWrapper::start()
{
	_timer.start();
}

void FramebufferWrapper::stop()
{
	_timer.stop();
}

bool FramebufferWrapper::screenInit()
{
	return _grabber.setupScreen();
}

void FramebufferWrapper::capture()
{
	_grabber.grabFrame(_screenshot);

	emit sig_screenshot(_screenshot);
}

void FramebufferWrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
