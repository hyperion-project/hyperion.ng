// Hyperion includes
#include <hyperion/GrabberWrapper.h>
#include <hyperion/Grabber.h>
#include <HyperionConfig.h>

// utils includes
#include <utils/GlobalSignals.h>

// qt
#include <QTimer>

GrabberWrapper::GrabberWrapper(QString grabberName, Grabber * ggrabber, unsigned width, unsigned height, const unsigned updateRate_Hz)
	: _grabberName(grabberName)
	, _timer(new QTimer(this))
	, _updateInterval_ms(1000/updateRate_Hz)
	, _log(Logger::getInstance(grabberName))
	, _ggrabber(ggrabber)
	, _image(0,0)
{
	// Configure the timer to generate events every n milliseconds
	_timer->setInterval(_updateInterval_ms);

	_image.resize(width, height);

	connect(_timer, &QTimer::timeout, this, &GrabberWrapper::action);

	// connect the image forwarding
	(_grabberName.startsWith("V4L"))
		? connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setV4lImage)
		: connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setSystemImage);
}

GrabberWrapper::~GrabberWrapper()
{
	GrabberWrapper::stop(); // TODO Is this right????????
	Debug(_log,"Close grabber: %s", QSTRING_CSTR(_grabberName));
}

bool GrabberWrapper::start()
{
	// Start the timer with the pre configured interval
	_timer->start();
	return _timer->isActive();
}

void GrabberWrapper::stop()
{
	// Stop the timer, effectivly stopping the process
	_timer->stop();
}

QStringList GrabberWrapper::availableGrabbers()
{
	QStringList grabbers;

	#ifdef ENABLE_DISPMANX
	grabbers << "dispmanx";
	#endif

	#ifdef ENABLE_V4L2
	grabbers << "v4l2";
	#endif

	#ifdef ENABLE_FB
	grabbers << "framebuffer";
	#endif

	#ifdef ENABLE_AMLOGIC
	grabbers << "amlogic";
	#endif

	#ifdef ENABLE_OSX
	grabbers << "osx";
	#endif

	#ifdef ENABLE_X11
	grabbers << "x11";
	#endif

	#ifdef ENABLE_QT
	grabbers << "qt";
	#endif

	return grabbers;
}


void GrabberWrapper::setVideoMode(const VideoMode& mode)
{
	if (_ggrabber != nullptr)
	{
		Info(_log,"setvideomode");
		_ggrabber->setVideoMode(mode);
	}
}

void GrabberWrapper::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	_ggrabber->setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void GrabberWrapper::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::V4L2 || type == settings::SYSTEMCAPTURE)
	{
		// extract settings
		const QJsonObject& obj = config.object();

		if(type == settings::SYSTEMCAPTURE  && !_grabberName.startsWith("V4L"))
		{
			// width/height
			_ggrabber->setWidthHeight(obj["width"].toInt(96), obj["height"].toInt(96));

			// display index for MAC
			_ggrabber->setDisplayIndex(obj["display"].toInt(0));

			// device path for Framebuffer
			_ggrabber->setDevicePath(obj["device"].toString("/dev/fb0"));

			// pixel decimation for x11
			_ggrabber->setPixelDecimation(obj["pixelDecimation"].toInt(8));

			// crop for system capture
			_ggrabber->setCropping(
				obj["cropLeft"].toInt(0),
				obj["cropRight"].toInt(0),
				obj["cropTop"].toInt(0),
				obj["cropBottom"].toInt(0));

			// eval new update timer (not for v4l)
			if(_updateInterval_ms != 1000/obj["frequency_Hz"].toInt(10))
			{
				_updateInterval_ms = 1000/obj["frequency_Hz"].toInt(10);
				const bool& timerWasActive = _timer->isActive();
				_timer->stop();
				_timer->setInterval(_updateInterval_ms);
				if(timerWasActive)
					_timer->start();
			}
		}

		// v4l instances only!
		if(type == settings::V4L2 && _grabberName.startsWith("V4L"))
		{
			// pixel decimation for v4l
			_ggrabber->setPixelDecimation(obj["sizeDecimation"].toInt(8));

			// crop for v4l
			_ggrabber->setCropping(
				obj["cropLeft"].toInt(0),
				obj["cropRight"].toInt(0),
				obj["cropTop"].toInt(0),
				obj["cropBottom"].toInt(0));

			_ggrabber->setSignalDetectionEnable(obj["signalDetection"].toBool(true));
			_ggrabber->setSignalDetectionOffset(
				obj["sDHOffsetMin"].toDouble(0.25),
				obj["sDVOffsetMin"].toDouble(0.25),
				obj["sDHOffsetMax"].toDouble(0.75),
				obj["sDVOffsetMax"].toDouble(0.75));
			_ggrabber->setSignalThreshold(
				obj["redSignalThreshold"].toDouble(0.0)/100.0,
				obj["greenSignalThreshold"].toDouble(0.0)/100.0,
				obj["blueSignalThreshold"].toDouble(0.0)/100.0);
			_ggrabber->setDeviceVideoStandard(
				obj["device"].toString("auto"),
				parseVideoStandard(obj["standard"].toString("no-change")));

		}
	}
}
