#include <QMetaType>

#include <grabber/VideoWrapper.h>

// qt includes
#include <QTimer>

VideoWrapper::VideoWrapper()
#if defined(ENABLE_V4L2)
	: GrabberWrapper("V4L2", &_grabber)
#elif defined(ENABLE_MF)
	: GrabberWrapper("V4L2:MEDIA_FOUNDATION", &_grabber)
#endif
	, _grabber()
{
	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	// Handle the image in the captured thread (Media Foundation/V4L2) using a direct connection
	connect(&_grabber, SIGNAL(newFrame(const Image<ColorRgb>&)), this, SLOT(newFrame(const Image<ColorRgb>&)), Qt::DirectConnection);
	connect(&_grabber, SIGNAL(readError(const char*)), this, SLOT(readError(const char*)), Qt::DirectConnection);
}

VideoWrapper::~VideoWrapper()
{
	stop();
}

bool VideoWrapper::start()
{
	return (_grabber.prepare() && _grabber.start() && GrabberWrapper::start());
}

void VideoWrapper::stop()
{
	_grabber.stop();
	GrabberWrapper::stop();
}

#if defined(ENABLE_CEC) && !defined(ENABLE_MF)

void VideoWrapper::handleCecEvent(CECEvent event)
{
	_grabber.handleCecEvent(event);
}

#endif

void VideoWrapper::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::V4L2 && _grabberName.startsWith("V4L2"))
	{
		// extract settings
		const QJsonObject& obj = config.object();

		// set global grabber state
		setV4lGrabberState(obj["enable"].toBool(false));

		if (getV4lGrabberState())
		{
#if defined(ENABLE_MF)
			// Device path
			_grabber.setDevice(obj["device"].toString("none"));
#endif

#if defined(ENABLE_V4L2)
			// Device path and name
			_grabber.setDevice(obj["device"].toString("none"), obj["available_devices"].toString("none"));
#endif

			// Device input
			_grabber.setInput(obj["input"].toInt(0));

			// Device resolution
			_grabber.setWidthHeight(obj["width"].toInt(0), obj["height"].toInt(0));

			// Device encoding format
			_grabber.setEncoding(obj["encoding"].toString("NO_CHANGE"));

			// Video standard
			_grabber.setVideoStandard(parseVideoStandard(obj["standard"].toString("NO_CHANGE")));

			// Image size decimation
			_grabber.setPixelDecimation(obj["sizeDecimation"].toInt(8));

			// Flip mode
			_grabber.setFlipMode(parseFlipMode(obj["flip"].toString("NO_CHANGE")));

			// Image cropping
			_grabber.setCropping(
				obj["cropLeft"].toInt(0),
				obj["cropRight"].toInt(0),
				obj["cropTop"].toInt(0),
				obj["cropBottom"].toInt(0));

			// Brightness, Contrast, Saturation, Hue
			_grabber.setBrightnessContrastSaturationHue(
				obj["hardware_brightness"].toInt(0),
				obj["hardware_contrast"].toInt(0),
				obj["hardware_saturation"].toInt(0),
				obj["hardware_hue"].toInt(0));

#if defined(ENABLE_CEC) && defined(ENABLE_V4L2)
			// CEC Standby
			_grabber.setCecDetectionEnable(obj["cecDetection"].toBool(true));
#endif

			// Software frame skipping
			_grabber.setFpsSoftwareDecimation(obj["fpsSoftwareDecimation"].toInt(1));

			// Signal detection
			_grabber.setSignalDetectionEnable(obj["signalDetection"].toBool(true));
			_grabber.setSignalDetectionOffset(
				obj["sDHOffsetMin"].toDouble(0.25),
				obj["sDVOffsetMin"].toDouble(0.25),
				obj["sDHOffsetMax"].toDouble(0.75),
				obj["sDVOffsetMax"].toDouble(0.75));
			_grabber.setSignalThreshold(
				obj["redSignalThreshold"].toDouble(0.0)/100.0,
				obj["greenSignalThreshold"].toDouble(0.0)/100.0,
				obj["blueSignalThreshold"].toDouble(0.0)/100.0,
				obj["noSignalCounterThreshold"].toInt(50));

			// Device framerate
			_grabber.setFramerate(obj["fps"].toInt(15));

			updateTimer(_ggrabber->getUpdateInterval());

			// Reload the Grabber if any settings have been changed that require it
			_grabber.reload(getV4lGrabberState());
		}
		else
			stop();
	}
}

void VideoWrapper::newFrame(const Image<ColorRgb> &image)
{
	emit systemImage(_grabberName, image);
}

void VideoWrapper::readError(const char* err)
{
	Error(_log, "Stop grabber, because reading device failed. (%s)", err);
	stop();
}

void VideoWrapper::action()
{
	// dummy as v4l get notifications from stream
}
