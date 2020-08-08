
// Hyperion-AmLogic includes
#include "FramebufferWrapper.h"

FramebufferWrapper::FramebufferWrapper(const QString & device, unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz) :
	_timer(this),
	_grabber(device,grabWidth, grabHeight)
{
	_timer.setTimerType(Qt::PreciseTimer);
	_timer.setSingleShot(false);
	_timer.setInterval(updateRate_Hz);

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

void FramebufferWrapper::capture()
{
	_grabber.grabFrame(_screenshot);

	emit sig_screenshot(_screenshot);
}
