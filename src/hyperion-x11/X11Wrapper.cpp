
#include "X11Wrapper.h"

X11Wrapper::X11Wrapper( int updateRate_Hz,
						int pixelDecimation,
						int cropLeft, int cropRight,
						int cropTop, int cropBottom
						) :
	  _timer(this),
	  _grabber(cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setFramerate(updateRate_Hz);
	_grabber.setPixelDecimation(pixelDecimation);

	_timer.setTimerType(Qt::PreciseTimer);
	_timer.setSingleShot(false);
	_timer.setInterval(_grabber.getUpdateInterval());

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
	return _grabber.setupDisplay();
}

void X11Wrapper::capture()
{
	_grabber.grabFrame(_screenshot, !_inited);
	emit sig_screenshot(_screenshot);
	_inited = true;
}

void X11Wrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
