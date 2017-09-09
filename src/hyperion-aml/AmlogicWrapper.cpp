
// Hyperion-AmLogic includes
#include "AmlogicWrapper.h"

AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz) :
	_timer(this),
	_grabber(grabWidth, grabHeight)
{
	_timer.setSingleShot(false);
	_timer.setInterval(updateRate_Hz);

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

void AmlogicWrapper::capture()
{
	_grabber.grabFrame(_screenshot);
	emit sig_screenshot(_screenshot);
}
