
// Hyperion-AmLogic includes
#include "AmlogicWrapper.h"

// Linux includes
#include <unistd.h>

AmlogicWrapper::AmlogicWrapper(unsigned grabWidth, unsigned grabHeight) :
	_thread(this),
	_grabber(grabWidth, grabHeight)
{
	_thread.setObjectName("AmlogicWrapperThread");

	// Connect capturing to the timeout signal of the timer
	connect(&_thread, SIGNAL (started()), this, SLOT(capture()));
}

const Image<ColorRgb> & AmlogicWrapper::getScreenshot()
{
	capture();
	return _screenshot;
}

void AmlogicWrapper::start()
{
	_thread.start();
}

void AmlogicWrapper::stop()
{
	_thread.quit();
}

void AmlogicWrapper::capture()
{
	while (_thread.isRunning())
	{
		_grabber.grabFrame(_screenshot);
		emit sig_screenshot(_screenshot);
		usleep(1 * 1000);
	}
}
