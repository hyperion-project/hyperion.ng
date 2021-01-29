#include <QMetaType>

#include <grabber/MFWrapper.h>

// qt
#include <QTimer>

MFWrapper::MFWrapper(const QString &device, unsigned grabWidth, unsigned grabHeight, unsigned fps, int pixelDecimation, QString flipMode)
	: GrabberWrapper("V4L2:MEDIA_FOUNDATION", &_grabber, grabWidth, grabHeight, 10)
	, _grabber(device, grabWidth, grabHeight, fps, pixelDecimation, flipMode)
{
	_ggrabber = &_grabber;

	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	// Handle the image in the captured thread using a direct connection
	connect(&_grabber, &MFGrabber::newFrame, this, &MFWrapper::newFrame, Qt::DirectConnection);
}

MFWrapper::~MFWrapper()
{
	stop();
}

bool MFWrapper::start()
{
	return ( _grabber.start() && GrabberWrapper::start());
}

void MFWrapper::stop()
{
	_grabber.stop();
	GrabberWrapper::stop();
}

void MFWrapper::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold)
{
	_grabber.setSignalThreshold( redSignalThreshold, greenSignalThreshold, blueSignalThreshold, noSignalCounterThreshold);
}

void MFWrapper::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	_grabber.setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void MFWrapper::setSignalDetectionOffset(double verticalMin, double horizontalMin, double verticalMax, double horizontalMax)
{
	_grabber.setSignalDetectionOffset(verticalMin, horizontalMin, verticalMax, horizontalMax);
}

void MFWrapper::newFrame(const Image<ColorRgb> &image)
{
	emit systemImage(_grabberName, image);
}

void MFWrapper::action()
{
	// dummy
}

void MFWrapper::setSignalDetectionEnable(bool enable)
{
	_grabber.setSignalDetectionEnable(enable);
}

bool MFWrapper::getSignalDetectionEnable() const
{
	return _grabber.getSignalDetectionEnabled();
}

void MFWrapper::setCecDetectionEnable(bool enable)
{
	_grabber.setCecDetectionEnable(enable);
}

bool MFWrapper::getCecDetectionEnable() const
{
	return _grabber.getCecDetectionEnabled();
}

bool MFWrapper::setDevice(const QString& device)
{
	return _grabber.setDevice(device);
}

void MFWrapper::setFpsSoftwareDecimation(int decimation)
{
	_grabber.setFpsSoftwareDecimation(decimation);
}

bool MFWrapper::setEncoding(QString enc)
{
	return _grabber.setEncoding(enc);
}

bool MFWrapper::setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue)
{
	return _grabber.setBrightnessContrastSaturationHue(brightness, contrast, saturation, hue);
}

void MFWrapper::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::V4L2 && _grabberName.startsWith("V4L2"))
	{
		// extract settings
		const QJsonObject& obj = config.object();
		// reload state
		bool reload = false;

		// device name, video standard
		if (_grabber.setDevice(obj["device"].toString("auto")))
			reload = true;

		// device input
		_grabber.setInput(obj["input"].toInt(-1));

		// device resolution
		if (_grabber.setWidthHeight(obj["width"].toInt(0), obj["height"].toInt(0)))
			reload = true;

		// device framerate
		if (_grabber.setFramerate(obj["fps"].toInt(15)))
			reload = true;

		// image size decimation
		_grabber.setPixelDecimation(obj["sizeDecimation"].toInt(8));

		// flip mode
		_grabber.setFlipMode(obj["flip"].toString("no-change"));

		// image cropping
		_grabber.setCropping(
			obj["cropLeft"].toInt(0),
			obj["cropRight"].toInt(0),
			obj["cropTop"].toInt(0),
			obj["cropBottom"].toInt(0));

		// Brightness, Contrast, Saturation, Hue
		if (_grabber.setBrightnessContrastSaturationHue(obj["hardware_brightness"].toInt(0), obj["hardware_contrast"].toInt(0), obj["hardware_saturation"].toInt(0), obj["hardware_hue"].toInt(0)))
			reload = true;

		// CEC Standby
		_grabber.setCecDetectionEnable(obj["cecDetection"].toBool(true));

		// software frame skipping
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
			obj["noSignalCounterThreshold"].toInt(50) );

		// Hardware encoding format
		if (_grabber.setEncoding(obj["encoding"].toString("NO_CHANGE")))
			reload = true;

		// Reload the Grabber if any settings have been changed that require it
		if (reload)
			_grabber.reloadGrabber();
	}
}
