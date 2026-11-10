#include "WLWrapper.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

namespace {
constexpr int SCREENSHOT_TIMEOUT_MS = 60000;
constexpr int SCREENSHOT_POLL_MS = 50;
}

WLWrapper::WLWrapper(int updateRate_Hz,
					 int pixelDecimation,
					 int cropLeft, int cropRight,
					 int cropTop, int cropBottom)
	: _timer(this)
	, _grabber(cropLeft, cropRight, cropTop, cropBottom)
{
	_grabber.setFramerate(updateRate_Hz);
	_grabber.setPixelDecimation(pixelDecimation);

	_timer.setTimerType(Qt::PreciseTimer);
	_timer.setSingleShot(false);
	_timer.setInterval(_grabber.getUpdateInterval());

	connect(&_timer, &QTimer::timeout, this, &WLWrapper::capture);
}

WLWrapper::~WLWrapper()
{
	_grabber.close();
}

const Image<ColorRgb>& WLWrapper::getScreenshot()
{
	if (_grabber.open())
	{
		QElapsedTimer waited;
		waited.start();
		while (_grabber.grabFrame(_screenshot) < 0 && !waited.hasExpired(SCREENSHOT_TIMEOUT_MS))
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, SCREENSHOT_POLL_MS);
			QThread::msleep(SCREENSHOT_POLL_MS);
		}
		_grabber.close();
	}
	return _screenshot;
}

void WLWrapper::start()
{
	if (_grabber.open())
	{
		_timer.start();
	}
}

void WLWrapper::stop()
{
	_timer.stop();
	_grabber.close();
}

bool WLWrapper::screenInit()
{
	return _grabber.isAvailable(true);
}

void WLWrapper::capture()
{
	// Returns -1 while the portal/user has not yet delivered a frame
	if (_grabber.grabFrame(_screenshot) >= 0)
	{
		emit sig_screenshot(_screenshot);
	}
}

void WLWrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}
