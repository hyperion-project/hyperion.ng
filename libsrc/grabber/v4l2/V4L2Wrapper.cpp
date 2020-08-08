#include <QMetaType>

#include <grabber/V4L2Wrapper.h>

// qt
#include <QTimer>

V4L2Wrapper::V4L2Wrapper(const QString &device,
		const unsigned grabWidth,
		const unsigned grabHeight,
		const unsigned fps,
		const unsigned input,
		VideoStandard videoStandard,
		PixelFormat pixelFormat,
		int pixelDecimation )
	: GrabberWrapper("V4L2:"+device, &_grabber, grabWidth, grabHeight, 10)
	, _grabber(device,
			grabWidth,
			grabHeight,
			fps,
			input,
			videoStandard,
			pixelFormat,
			pixelDecimation)
{
	_ggrabber = &_grabber;

	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	// Handle the image in the captured thread using a direct connection
	connect(&_grabber, &V4L2Grabber::newFrame, this, &V4L2Wrapper::newFrame, Qt::DirectConnection);
	connect(&_grabber, &V4L2Grabber::readError, this, &V4L2Wrapper::readError, Qt::DirectConnection);
}

V4L2Wrapper::~V4L2Wrapper()
{
	stop();
}

bool V4L2Wrapper::start()
{
	return ( _grabber.start() && GrabberWrapper::start());
}

void V4L2Wrapper::stop()
{
	_grabber.stop();
	GrabberWrapper::stop();
}

void V4L2Wrapper::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold)
{
	_grabber.setSignalThreshold( redSignalThreshold, greenSignalThreshold, blueSignalThreshold, 50);
}

void V4L2Wrapper::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	_grabber.setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void V4L2Wrapper::setSignalDetectionOffset(double verticalMin, double horizontalMin, double verticalMax, double horizontalMax)
{
	_grabber.setSignalDetectionOffset(verticalMin, horizontalMin, verticalMax, horizontalMax);
}

void V4L2Wrapper::newFrame(const Image<ColorRgb> &image)
{
	emit systemImage(_grabberName, image);
}

void V4L2Wrapper::readError(const char* err)
{
	Error(_log, "stop grabber, because reading device failed. (%s)", err);
	stop();
}

void V4L2Wrapper::action()
{
	// dummy as v4l get notifications from stream
}

void V4L2Wrapper::setSignalDetectionEnable(bool enable)
{
	_grabber.setSignalDetectionEnable(enable);
}

bool V4L2Wrapper::getSignalDetectionEnable()
{
	return _grabber.getSignalDetectionEnabled();
}

void V4L2Wrapper::setCecDetectionEnable(bool enable)
{
	_grabber.setCecDetectionEnable(enable);
}

bool V4L2Wrapper::getCecDetectionEnable()
{
	return _grabber.getCecDetectionEnabled();
}

void V4L2Wrapper::setDeviceVideoStandard(QString device, VideoStandard videoStandard)
{
	_grabber.setDeviceVideoStandard(device, videoStandard);
}

void V4L2Wrapper::handleCecEvent(CECEvent event)
{
	_grabber.handleCecEvent(event);
}

void V4L2Wrapper::loadLutFile(QString path)
{
	_grabber.loadLutFile(path);
}

void V4L2Wrapper::setHdrToneMappingEnabled(bool enable)
{
	_grabber.setHdrToneMappingEnabled(enable);
}

void V4L2Wrapper::setFpsSoftwareDecimation(int decimation)
{
	_grabber.setFpsSoftwareDecimation(decimation);
}

void V4L2Wrapper::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::V4L2 && _grabberName.startsWith("V4L"))
	{
		// extract settings
		const QJsonObject& obj = config.object();

		// pixel decimation for v4l
		_grabber.setPixelDecimation(obj["sizeDecimation"].toInt(8));

		// crop for v4l
		_grabber.setCropping(
			obj["cropLeft"].toInt(0),
			obj["cropRight"].toInt(0),
			obj["cropTop"].toInt(0),
			obj["cropBottom"].toInt(0));

		// device input
		_grabber.setInput(obj["input"].toInt(-1));

		// device resolution
		_grabber.setWidthHeight(obj["width"].toInt(0), obj["height"].toInt(0));

		// device framerate
		_grabber.setFramerate(obj["fps"].toInt(15));

		// CEC Standby
		_grabber.setCecDetectionEnable(obj["cecDetection"].toBool(true));

		// HDR tone mapping
		_grabber.setHdrToneMappingEnabled(obj["hdrToneMapping"].toBool(false));
		
		// software frame skipping
		_grabber.setFpsSoftwareDecimation(obj["fpsSoftwareDecimation"].toInt(1));
		
		_grabber.setSignalDetectionEnable(obj["signalDetection"].toBool(true));
		_grabber.setSignalDetectionOffset(
			obj["sDHOffsetMin"].toDouble(0.25),
			obj["sDVOffsetMin"].toDouble(0.25),
			obj["sDHOffsetMax"].toDouble(0.75),
			obj["sDVOffsetMax"].toDouble(0.75));
		_grabber.setSignalThreshold(
			obj["redSignalThreshold"].toDouble(0.0)/100.0,
			obj["greenSignalThreshold"].toDouble(0.0)/100.0,
			obj["blueSignalThreshold"].toDouble(0.0)/100.0);
		_grabber.setDeviceVideoStandard(
			obj["device"].toString("auto"),
			parseVideoStandard(obj["standard"].toString("no-change")));
	}
}
