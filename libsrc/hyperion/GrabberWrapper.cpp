// Hyperion includes
#include <hyperion/GrabberWrapper.h>
#include <hyperion/Grabber.h>
#include <HyperionConfig.h>

// utils includes
#include <utils/GlobalSignals.h>
#include <events/EventHandler.h>
#include <events/OsEventHandler.h>

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
	: _log(Logger::getInstance(("Grabber-" + grabberName).toUpper()))
	, _ggrabber(ggrabber)
	, _grabberName(grabberName)
	, _timer(nullptr)
	, _updateInterval_ms(1000/updateRate_Hz)
{
	TRACK_SCOPE();
	GrabberWrapper::instance = this;

	_timer.reset(new QTimer());

	// Configure the timer to generate events every n milliseconds
	_timer->setTimerType(Qt::PreciseTimer);
	_timer->setInterval(_updateInterval_ms);

	connect(_timer.get(), &QTimer::timeout, this, &GrabberWrapper::action);

	// connect the image forwarding
	if (_grabberName.startsWith("V4L"))
	{
		connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setV4lImage);
	}
	else if (_grabberName.startsWith("Audio"))
	{
		connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setAudioImage);
	}
	else
	{
		connect(this, &GrabberWrapper::systemImage, GlobalSignals::getInstance(), &GlobalSignals::setSystemImage);
	}

	// listen for source requests
	connect(GlobalSignals::getInstance(), &GlobalSignals::requestSource, this, &GrabberWrapper::handleSourceRequest);

	QObject::connect(EventHandler::getInstance().data(), &EventHandler::signalEvent, this, &GrabberWrapper::handleEvent);
}

GrabberWrapper::~GrabberWrapper()
{
	TRACK_SCOPE();
	_timer->stop();
	GrabberWrapper::instance = nullptr;
}

bool GrabberWrapper::start()
{
	if (!_ggrabber->isAvailable())
	{
		return false;
	}

	if ( !open() )
	{
		return false;
	}
	
	if (!_timer->isActive())
	{
		// Start the timer with the pre configured interval
		Info(_log,"%s grabber started", QSTRING_CSTR(getName()));
		_timer->start();
	}

	return _timer->isActive();
}

void GrabberWrapper::stop()
{
	if (_timer->isActive())
	{
		// Stop the timer, effectively stopping the process
		Info(_log,"%s grabber stopped", QSTRING_CSTR(getName()));
		_timer->stop();
	}
}

void GrabberWrapper::handleEvent(Event event)
{
	if (_ggrabber != nullptr)
	{
		_ggrabber->handleEvent(event);
	}
}

bool GrabberWrapper::isActive() const
{
	return _timer->isActive();
}

QStringList GrabberWrapper::getActive(int inst, GrabberTypeFilter type) const
{
	auto result = QStringList();

	if (type == GrabberTypeFilter::SCREEN || type == GrabberTypeFilter::ALL)
	{
		if (GRABBER_SYS_CLIENTS.contains(inst))
		{
			result << GRABBER_SYS_CLIENTS.value(inst);
		}
	}

	if (type == GrabberTypeFilter::VIDEO || type == GrabberTypeFilter::ALL)
	{
		if (GRABBER_V4L_CLIENTS.contains(inst))
		{
			result << GRABBER_V4L_CLIENTS.value(inst);
		}
	}

	if (type == GrabberTypeFilter::AUDIO || type == GrabberTypeFilter::ALL)
	{
		if (GRABBER_AUDIO_CLIENTS.contains(inst))
		{
			result << GRABBER_AUDIO_CLIENTS.value(inst);
		}
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

		#ifdef ENABLE_DDA
				grabbers << "dda";
		#endif

		#ifdef ENABLE_DRM
				grabbers << "drm";
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
		{
			_timer->start();
		}
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

			_ggrabber->setInput(obj["input"].toInt(0));

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
		qCDebug(grabber_screen_flow) << "Instance:" << hyperionInd << ", listen" << listen << ", Global screen grabber current state:" << getSysGrabberState();;
		if (listen)
		{
			GRABBER_SYS_CLIENTS.insert(hyperionInd, _grabberName);
			qCDebug(grabber_screen_flow) << "Adding screen grabber" << _grabberName << "to instance [" << hyperionInd << "]";
			if (GRABBER_SYS_CLIENTS.size() == 1)
			{
				Debug(_log, "First instance available for grabber");
			}
		}
		else
		{
			qCDebug(grabber_screen_flow) << "Removing screen grabber" << GRABBER_SYS_CLIENTS[hyperionInd] << "from instance [" << hyperionInd << "]";
			GRABBER_SYS_CLIENTS.remove(hyperionInd);
			if (GRABBER_SYS_CLIENTS.empty() || !getSysGrabberState())
			{
				Debug(_log, "Stop screen grabber %s, as no instance is listing any longer", QSTRING_CSTR(_grabberName));
				stop();
			}
		}
	}
	else if (component == hyperion::Components::COMP_V4L &&
		_grabberName.startsWith("V4L"))
	{
		qCDebug(grabber_video_flow) << "Instance:" << hyperionInd << ", listen" << listen << ", Global video grabber current state:" << getV4lGrabberState();;

		if (listen)
		{
			GRABBER_V4L_CLIENTS.insert(hyperionInd, _grabberName);
			qCDebug(grabber_video_flow) << "Adding video grabber" << _grabberName << "to instance [" << hyperionInd << "]";
			if (GRABBER_V4L_CLIENTS.size() == 1)
			{
				Debug(_log, "Start video grabber on first instance listing");
				start();
			}
		}
		else
		{
			qCDebug(grabber_video_flow) << "Removing video grabber" << GRABBER_V4L_CLIENTS[hyperionInd] << "from instance [" << hyperionInd << "]";
			GRABBER_V4L_CLIENTS.remove(hyperionInd);
			if (GRABBER_V4L_CLIENTS.empty() || !getV4lGrabberState())
			{
				Debug(_log, "Stop video grabber %s, as no instance is listing any longer", QSTRING_CSTR(_grabberName));
				stop();
			}
		}
	}
	else if (component == hyperion::Components::COMP_AUDIO &&
		_grabberName.startsWith("Audio"))
	{
		qCDebug(grabber_audio_flow) << "Instance:" << hyperionInd << ", listen" << listen << ", Global audio grabber current state:" << getAudioGrabberState();;

		if (listen)
		{
			qCDebug(grabber_audio_flow) << "Adding audio grabber" << _grabberName << "to instance [" << hyperionInd << "]";
			GRABBER_AUDIO_CLIENTS.insert(hyperionInd, _grabberName);
			if (GRABBER_AUDIO_CLIENTS.size() == 1)
			{
				Debug(_log, "Start audio grabber on first instance listing");
				start();
			}
		}
		else
		{
			qCDebug(grabber_audio_flow) << "Removing audio grabber" << GRABBER_AUDIO_CLIENTS[hyperionInd] << "from instance [" << hyperionInd << "]";
			GRABBER_AUDIO_CLIENTS.remove(hyperionInd);
			if (GRABBER_AUDIO_CLIENTS.empty() || !getAudioGrabberState())
			{
				Debug(_log, "Stop audio grabber %s, as no instance is listing any longer", QSTRING_CSTR(_grabberName));
				stop();
			}
		}
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
