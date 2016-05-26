
// Hyperion-AmLogic includes
#include "OsxWrapper.h"

OsxWrapper::OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz) :
	_timer(this),
	_grabber(display,grabWidth, grabHeight)
{
	_timer.setSingleShot(false);
	_timer.setInterval(updateRate_Hz);

	// Connect capturing to the timeout signal of the timer
	connect(&_timer, SIGNAL(timeout()), this, SLOT(capture()));
}

const Image<ColorRgb> & OsxWrapper::getScreenshot()
{
	capture();
	return _screenshot;
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
