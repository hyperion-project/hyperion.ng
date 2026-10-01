#include "V4L2Wrapper.h"

#include <algorithm>

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>

#include "ScreenshotHandler.h"

V4L2Wrapper::V4L2Wrapper(const V4L2GrabberOptions& opts)
{
	// Required for the queued connection used by V4L2Grabber's decoder threads
	// (EncoderThreadManager::newFrame -> V4L2Grabber::newThreadFrame) to deliver images
	// across thread boundaries.
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	// V4L2Grabber::init() only succeeds if the device name passed to setDevice() exactly
	// matches the name V4L2Grabber itself enumerates for that device path (and never
	// succeeds at all for the literal path "auto"). Since the CLI only lets the user
	// specify a device *path* (there is no --device-name option), resolve the matching
	// (path, name) pair via discover() up front -- for "auto" this picks the first
	// discovered device, implementing the "auto detected" behaviour the --device help
	// text advertises.
	QString resolvedPath;
	QString resolvedName;
	const bool autoDetect = opts.device.compare(QStringLiteral("auto"), Qt::CaseInsensitive) == 0;
	const QJsonArray discovered = _grabber.discover(QJsonObject());
	for (const QJsonValue& entry : discovered)
	{
		const QJsonObject obj = entry.toObject();
		if (autoDetect || obj.value("device").toString() == opts.device)
		{
			resolvedPath = obj.value("device").toString();
			resolvedName = obj.value("device_name").toString();
			_deviceResolved = true;
			break;
		}
	}

	_grabber.setDevice(_deviceResolved ? resolvedPath : opts.device, resolvedName);
	_grabber.setInput(opts.input);
	_grabber.setWidthHeight(opts.width, opts.height);
	_grabber.setFramerate(opts.fps);

	if (opts.pixelFormat != PixelFormat::NO_CHANGE)
	{
		_grabber.setEncoding(pixelFormatToString(opts.pixelFormat));
	}

	if (opts.videoStandard != VideoStandard::NO_CHANGE)
	{
		_grabber.setVideoStandard(opts.videoStandard);
	}

	_grabber.setPixelDecimation(std::max(1, opts.sizeDecimation));
	_grabber.setFlipMode(opts.flipMode);

	_grabber.setSignalDetectionEnable(opts.signalDetectionEnabled);
	_grabber.setSignalThreshold(opts.signalThresholdRed, opts.signalThresholdGreen, opts.signalThresholdBlue, 50);

	_grabber.setCropping(opts.cropLeft, opts.cropRight, opts.cropTop, opts.cropBottom);

	if (opts.applySignalDetectionOffset)
	{
		_grabber.setSignalDetectionOffset(opts.signalHorizontalMin, opts.signalVerticalMin, opts.signalHorizontalMax, opts.signalVerticalMax);
	}

	connect(&_grabber, &V4L2Grabber::newFrame, this, &V4L2Wrapper::sig_screenshot);
}

bool V4L2Wrapper::screenInit()
{
	if (!_deviceResolved)
	{
		return false;
	}

	// Device opening itself is deferred to start() (same as the original hyperion-v4l2
	// main()); prepare() only allocates the decoder thread manager.
	return _grabber.prepare();
}

void V4L2Wrapper::setVideoMode(VideoMode mode)
{
	_grabber.setVideoMode(mode);
}

const Image<ColorRgb>& V4L2Wrapper::getScreenshot()
{
	QEventLoop loop;

	// Prints the same "no signal area" suggestion diagnostics to stdout as before.
	ScreenshotHandler handler(_grabber.getSignalDetectionOffset());
	connect(&_grabber, &V4L2Grabber::newFrame, &handler, &ScreenshotHandler::receiveImage);

	QMetaObject::Connection const captureConn = connect(&_grabber, &V4L2Grabber::newFrame, this, [this, &loop](const Image<ColorRgb>& image) {
		_screenshot = image;
		loop.quit();
	});

	_grabber.start();
	loop.exec();
	_grabber.stop();

	disconnect(captureConn);

	return _screenshot;
}

void V4L2Wrapper::start()
{
	_grabber.start();
}

void V4L2Wrapper::stop()
{
	_grabber.stop();
}
