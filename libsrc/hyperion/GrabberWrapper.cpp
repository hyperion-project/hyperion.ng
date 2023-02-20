// Hyperion includes
#include <hyperion/GrabberWrapper.h>
#include <hyperion/Grabber.h>
#include <HyperionConfig.h>

// utils includes
#include <utils/GlobalSignals.h>

// qt
#include <QTimer>

GrabberWrapper* GrabberWrapper::instance = nullptr;
const int GrabberWrapper::DEFAULT_RATE_HZ = 25;
const int GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ = 1;
const int GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ = 30;
const int GrabberWrapper::DEFAULT_PIXELDECIMATION = 8;

/// Map of Hyperion instances with grabber name that requested screen capture
QMap<int, QString> GrabberWrapper::GRABBER_SYS_CLIENTS = QMap<int, QString>();
QMap<int, QString> GrabberWrapper::GRABBER_V4L_CLIENTS = QMap<int, QString>();
QMap<int, QString> GrabberWrapper::GRABBER_AUDIO_CLIENTS = QMap<int, QString>();
bool GrabberWrapper::GLOBAL_GRABBER_SYS_ENABLE = false;
bool GrabberWrapper::GLOBAL_GRABBER_V4L_ENABLE = false;
bool GrabberWrapper::GLOBAL_GRABBER_AUDIO_ENABLE = false;

GrabberWrapper::GrabberWrapper(const QString& grabberName, Grabber * ggrabber, int updateRate_Hz)
	: _grabberName(grabberName)
	  , _log(Logger::getInstance(grabberName.toUpper()))
	  , _timer(new QTimer(this))
	  , _updateInterval_ms(1000/updateRate_Hz)
	  , _ggrabber(ggrabber)
	  , _image(0,0)
{
	GrabberWrapper::instance = this;

	// Configure the timer to generate events every n milliseconds
	_timer->setTimerType(Qt::PreciseTimer);
	_timer->setInterval(_updateInterval_ms);

	connect(_timer, &QTimer::timeout, this, &GrabberWrapper::action);

	// connect the image forwarding
	if (_grabberName.startsWith("V4L"))
		connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setV4lImage);
	else if (_grabberName.startsWith("Audio"))
		connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setAudioImage);
	else
		connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setSystemImage);

	// listen for source requests
	connect(GlobalSignals::getInstance(), &GlobalSignals::requestSource, this, &GrabberWrapper::handleSourceRequest);
}

GrabberWrapper::~GrabberWrapper()
{
	GrabberWrapper::instance = nullptr;
}

bool GrabberWrapper::start()
{
	bool rc = false;
	if ( open() )
	{
		if (!_timer->isActive())
		{
			// Start the timer with the pre configured interval
			Debug(_log,"Grabber start()");
			_timer->start();
		}

		rc = _timer->isActive();
	}
	return rc;
}

void GrabberWrapper::stop()
{
	if (_timer->isActive())
	{
		// Stop the timer, effectively stopping the process
		Debug(_log,"Grabber stop()");
		_timer->stop();
	}
}

bool GrabberWrapper::isActive() const
{
	return _timer->isActive();
}

QStringList GrabberWrapper::getActive(int inst, GrabberTypeFilter type) const
{
	QStringList result = QStringList();

	if (type == GrabberTypeFilter::SCREEN || type == GrabberTypeFilter::ALL)
	{
		if (GRABBER_SYS_CLIENTS.contains(inst))
			result << GRABBER_SYS_CLIENTS.value(inst);
	}

	if (type == GrabberTypeFilter::VIDEO || type == GrabberTypeFilter::ALL)
	{
		if (GRABBER_V4L_CLIENTS.contains(inst))
			result << GRABBER_V4L_CLIENTS.value(inst);
	}

	if (type == GrabberTypeFilter::AUDIO || type == GrabberTypeFilter::ALL)
	{
		if (GRABBER_AUDIO_CLIENTS.contains(inst))
			result << GRABBER_AUDIO_CLIENTS.value(inst);
	}

	return result;
}

QStringList GrabberWrapper::availableGrabbers(GrabberTypeFilter type)
{
	QStringList grabbers;

	if (type == GrabberTypeFilter::SCREEN || type == GrabberTypeFilter::ALL)
	{
		#ifdef ENABLE_DISPMANX
				grabbers << "dispmanx";
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

		#ifdef ENABLE_XCB
				grabbers << "xcb";
		#endif

		#ifdef ENABLE_QT
				grabbers << "qt";
		#endif

		#ifdef ENABLE_DX
				grabbers << "dx";
		#endif
	}

	if (type == GrabberTypeFilter::VIDEO || type == GrabberTypeFilter::ALL)
	{
		#if defined(ENABLE_V4L2) || defined(ENABLE_MF)
			grabbers << "v4l2";
		#endif
	}

	if (type == GrabberTypeFilter::AUDIO || type == GrabberTypeFilter::ALL)
	{
		#ifdef ENABLE_AUDIO
			grabbers << "audio";
		#endif
	}

	return grabbers;
}

void GrabberWrapper::setVideoMode(VideoMode mode)
{
	if (_ggrabber != nullptr)
	{
		Info(_log,"setVideoMode");
		_ggrabber->setVideoMode(mode);
	}
}

void GrabberWrapper::setFlipMode(const QString& flipMode)
{
	_ggrabber->setFlipMode(parseFlipMode(flipMode));
}

void GrabberWrapper::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	_ggrabber->setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void GrabberWrapper::updateTimer(int interval)
{
	if(_updateInterval_ms != interval)
	{
		_updateInterval_ms = interval;

		const bool& timerWasActive = _timer->isActive();
		_timer->stop();
		_timer->setInterval(_updateInterval_ms);

		if(timerWasActive)
			_timer->start();
	}
}

void GrabberWrapper::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::SYSTEMCAPTURE &&
		!_grabberName.startsWith("V4L") &&
		!_grabberName.startsWith("Audio"))
	{
		// extract settings
		const QJsonObject& obj = config.object();

		// save current state
		bool isEnabled = getSysGrabberState();

		// set global grabber state
		setSysGrabberState(obj["enable"].toBool(false));

		if (getSysGrabberState())
		{
			// width/height
			_ggrabber->setWidthHeight(obj["width"].toInt(96), obj["height"].toInt(96));

			// display index for MAC
			_ggrabber->setDisplayIndex(obj["input"].toInt(0));

			// pixel decimation for x11
			_ggrabber->setPixelDecimation(obj["pixelDecimation"].toInt(DEFAULT_PIXELDECIMATION));

			// crop for system capture
			_ggrabber->setCropping(
				obj["cropLeft"].toInt(0),
				obj["cropRight"].toInt(0),
				obj["cropTop"].toInt(0),
				obj["cropBottom"].toInt(0));

			_ggrabber->setFramerate(obj["fps"].toInt(DEFAULT_RATE_HZ));
			// eval new update time
			updateTimer(_ggrabber->getUpdateInterval());

			// start if current state is not true
			if (!isEnabled)
			{
				start();
			}
		}
		else
		{
			stop();
		}
	}
}

void GrabberWrapper::handleSourceRequest(hyperion::Components component, int hyperionInd, bool listen)
{
	if (component == hyperion::Components::COMP_GRABBER &&
		!_grabberName.startsWith("V4L") &&
		!_grabberName.startsWith("Audio"))
	{
		if (listen)
			GRABBER_SYS_CLIENTS.insert(hyperionInd, _grabberName);
		else
			GRABBER_SYS_CLIENTS.remove(hyperionInd);

		if (GRABBER_SYS_CLIENTS.empty() || !getSysGrabberState())
			stop();
		else
			start();
	}
	else if (component == hyperion::Components::COMP_V4L &&
		_grabberName.startsWith("V4L"))
	{
		if (listen)
			GRABBER_V4L_CLIENTS.insert(hyperionInd, _grabberName);
		else
			GRABBER_V4L_CLIENTS.remove(hyperionInd);

		if (GRABBER_V4L_CLIENTS.empty() || !getV4lGrabberState())
			stop();
		else
			start();
	}
	else if (component == hyperion::Components::COMP_AUDIO &&
		_grabberName.startsWith("Audio"))
	{
		if (listen)
			GRABBER_AUDIO_CLIENTS.insert(hyperionInd, _grabberName);
		else
			GRABBER_AUDIO_CLIENTS.remove(hyperionInd);

		if (GRABBER_AUDIO_CLIENTS.empty())
			stop();
		else
			start();
	}
}

void GrabberWrapper::tryStart()
{
	// verify start condition
	if (!_grabberName.startsWith("V4L") &&
		!_grabberName.startsWith("Audio") &&
		!GRABBER_SYS_CLIENTS.empty() &&
		getSysGrabberState())
	{
		start();
	}
}
