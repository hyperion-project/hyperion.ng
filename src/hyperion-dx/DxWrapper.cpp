
// Hyperion-qt includes
#include "DxWrapper.h"

DxWrapper::DxWrapper(int grabInterval, int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display) :
	_timer(this),
	_grabber(cropLeft, cropRight, cropTop, cropBottom, pixelDecimation, display)
{
	_timer.setInterval(grabInterval);
	// Connect capturing to the timeout signal of the timer
	connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & DxWrapper::getScreenshot()
{
	_grabber.grabFrame(_screenshot);
	return _screenshot;
}

void DxWrapper::start()
{
	_timer.start();
}

void DxWrapper::stop()
{
	_timer.stop();
}

void DxWrapper::capture()
{
	if(unsigned(_grabber.getImageWidth()) != unsigned(_screenshot.width()) || unsigned(_grabber.getImageHeight()) != unsigned(_screenshot.height()))
		_screenshot.resize(_grabber.getImageWidth(),_grabber.getImageHeight());

	_grabber.grabFrame(_screenshot);
	emit sig_screenshot(_screenshot);
}

void DxWrapper::setVideoMode(const VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
