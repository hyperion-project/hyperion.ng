
// Hyperion-qt includes
#include "QtWrapper.h"

QtWrapper::QtWrapper(int grabInterval, int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display) :
	_timer(this),
	_grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation, display)
{
	_timer.setInterval(grabInterval);
	// Connect capturing to the timeout signal of the timer
	connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & QtWrapper::getScreenshot()
{
	_grabber.grabFrame(_screenshot);
	return _screenshot;
}

void QtWrapper::start()
{
	_timer.start();
}

void QtWrapper::stop()
{
	_timer.stop();
}

void QtWrapper::capture()
{
	if(unsigned(_grabber.getImageWidth()) != unsigned(_screenshot.width()) || unsigned(_grabber.getImageHeight()) != unsigned(_screenshot.height()))
		_screenshot.resize(_grabber.getImageWidth(),_grabber.getImageHeight());

	_grabber.grabFrame(_screenshot);
	emit sig_screenshot(_screenshot);
}

void QtWrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
