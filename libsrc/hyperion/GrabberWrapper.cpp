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

Q_LOGGING_CATEGORY(grabber_flow, "hyperion.grabber.flow");

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
	qCDebug(grabber_flow) << "Request to start grabber" << _grabberName
		<< ", which is currently"<< (_ggrabber->isEnabled() ? "enabled" : "disabled");
	if (!_ggrabber->isAvailable())
	{
		qCDebug(grabber_flow) << "Grabber" << _grabberName << "is not started as it not available";
		return false;
	}

	if (!_ggrabber->isEnabled())
	{
		qCDebug(grabber_flow) << "Grabber" << _grabberName << "is not started as it is disabled";
		Info(_log,"%s grabber is disabled, it is not started", QSTRING_CSTR(getName()));
		return false;
	}

	if (isActive())
	{
		qCDebug(grabber_flow) << "Grabber" << _grabberName << "is already running";
		return true;
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
	qCDebug(grabber_flow) << "Grabber" << _grabberName << (_timer->isActive() ? "active" : "inactive") << "now";

	return _timer->isActive();
}

void GrabberWrapper::stop()
{
	qCDebug(grabber_flow) << "Request to stop grabber" << _grabberName
		<< ", which is currently"<< (_ggrabber->isEnabled() ? "enabled" : "disabled")
		<< ", and" << (_timer->isActive() ? "active" : "inactive");

	if (_timer->isActive())
	{
		// Stop the timer, effectively stopping the process
		Info(_log,"%s grabber stopped", QSTRING_CSTR(getName()));
		_timer->stop();
	}
	qCDebug(grabber_flow) << "Grabber" << _grabberName << "stopped";
}

bool GrabberWrapper::restart()
{
	qCDebug(grabber_flow) << "Request to restart grabber" << _grabberName
		<< ", which is currently" << (_ggrabber->isEnabled() ? "enabled" : "disabled")
		<< ", and" << (_timer->isActive() ? "active" : "inactive");

	_timer->stop();
	return start();
}

void GrabberWrapper::handleEvent(Event event)
{
	qCDebug(grabber_flow) << "Received event" << event << "for grabber" << _grabberName;
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
		const QJsonObject& obj = config.object();
		qCDebug(grabber_flow) << "Screen grabber" << _grabberName << "updating settings with" << obj;

		// save current state
		bool isCurrentlyEnabled = getSysGrabberState();

		// set global grabber state
		setSysGrabberState(obj["enable"].toBool(false));
		qCDebug(grabber_flow) << "Screen grabber" << _grabberName << "is configured" << (getSysGrabberState() ? "enabled" : "disabled") << ", current state is" << (isCurrentlyEnabled ? "enabled" : "disabled");

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

			// restart the grabber after configuration change
			Info(_log, "Restarting grabber %s after settings update", QSTRING_CSTR(_grabberName));
			_ggrabber->setEnabled(true);
			start();
		}
		else
		{
			_ggrabber->setEnabled(false);
			if (isCurrentlyEnabled)
			{
				Info(_log, "Stop running grabber %s after settings update", QSTRING_CSTR(_grabberName));
				stop();
			}
		}
	}
}

void GrabberWrapper::handleSourceRequestScreen(hyperion::Components component, int hyperionInd, bool listen)
{
	// Handle screen grabber requests (e.g., X11, DirectX)
	if (component == hyperion::Components::COMP_GRABBER &&
		!_grabberName.startsWith("V4L") &&
		!_grabberName.startsWith("Audio"))
	{
		qCDebug(grabber_screen_flow) << "Instance [" << hyperionInd << "] - Request to" << (listen ? "add" : "remove") << "screen grabber" << _grabberName << "which is" << (getSysGrabberState() ? "enabled" : "disabled");
		if (listen)
		{
			// If the screen grabber is requested
			if (GRABBER_SYS_CLIENTS.contains(hyperionInd) && GRABBER_SYS_CLIENTS[hyperionInd] == _grabberName)
			{
				// This instance is already listening to this grabber
				qCDebug(grabber_screen_flow) << "Instance [" << hyperionInd << "] - Screen grabber" << _grabberName << "is already registered";
				return;
			}

			// Add the instance to the list of clients for this grabber
			GRABBER_SYS_CLIENTS.insert(hyperionInd, _grabberName);
			qCDebug(grabber_screen_flow) << "Instance [" << hyperionInd << "] - Adding screen grabber" << _grabberName;
			if (GRABBER_SYS_CLIENTS.size() == 1)
			{
				// If this is the first client, start the grabber
				qCDebug(grabber_screen_flow) << "Instance [" << hyperionInd << "] - First instance available for screen grabber";
				start();
			}
		}
		else
		{
			// If the screen grabber is released
			qCDebug(grabber_screen_flow) << "Instance [" << hyperionInd << "] - Removing screen grabber" << GRABBER_SYS_CLIENTS[hyperionInd];
			GRABBER_SYS_CLIENTS.remove(hyperionInd);
			if (GRABBER_SYS_CLIENTS.empty() || !getSysGrabberState())
			{
				// If there are no more clients or the grabber is disabled, stop it
				Debug(_log, "Stop screen grabber %s, as no instance is listing any longer", QSTRING_CSTR(_grabberName));
				stop();
			}
		}
	}
}

void GrabberWrapper::handleSourceRequestVideo(hyperion::Components component, int hyperionInd, bool listen)
{
	if (component == hyperion::Components::COMP_V4L)
	{
		qCDebug(grabber_video_flow) << "Instance [" << hyperionInd << "] - Request to" << (listen ? "add" : "remove") << "video grabber" << _grabberName << "which is" << (getV4lGrabberState() ? "enabled" : "disabled");

		if (listen)
		{
			// If the video grabber is requested
			if (GRABBER_V4L_CLIENTS.contains(hyperionInd) && GRABBER_V4L_CLIENTS[hyperionInd] == _grabberName)
			{
				// This instance is already listening to this grabber
				qCDebug(grabber_video_flow) << "Instance [" << hyperionInd << "] - Video grabber" << _grabberName << "is already registered";
				return;
			}

			// Add the instance to the list of clients for this grabber
			GRABBER_V4L_CLIENTS.insert(hyperionInd, _grabberName);
			qCDebug(grabber_video_flow) << "Instance [" << hyperionInd << "] - Adding video grabber" << _grabberName;
			if (GRABBER_V4L_CLIENTS.size() == 1)
			{
				// If this is the first client, start the grabber
				qCDebug(grabber_video_flow) << "Instance [" << hyperionInd << "] - First instance available for video grabber";
				start();
			}
		}
		else
		{
			// If the video grabber is released
			qCDebug(grabber_video_flow) << "Removing video grabber" << GRABBER_V4L_CLIENTS[hyperionInd] << "from instance [" << hyperionInd << "]";
			GRABBER_V4L_CLIENTS.remove(hyperionInd);
			if (GRABBER_V4L_CLIENTS.empty() || !getV4lGrabberState())
			{
				// If there are no more clients or the grabber is disabled, stop it
				Debug(_log, "Stop video grabber %s, as no instance is listing any longer", QSTRING_CSTR(_grabberName));
				stop();
			}
		}
	}
}

void GrabberWrapper::handleSourceRequestAudio(hyperion::Components component, int hyperionInd, bool listen)
{
	if (component == hyperion::Components::COMP_AUDIO &&
		_grabberName.startsWith("Audio"))
	{
		qCDebug(grabber_audio_flow) << "Instance [" << hyperionInd << "] - Request to" << (listen ? "add" : "remove") << "audio grabber" << _grabberName << "which is" << (getAudioGrabberState() ? "enabled" : "disabled");

		if (listen)
		{
			// If the audio grabber is requested
			if (GRABBER_AUDIO_CLIENTS.contains(hyperionInd) && GRABBER_AUDIO_CLIENTS[hyperionInd] == _grabberName)
			{
				// This instance is already listening to this grabber
				qCDebug(grabber_audio_flow) << "Instance [" << hyperionInd << "] - Audio grabber" << _grabberName << "is already registered";
				return;
			}

			// Add the instance to the list of clients for this grabber
			qCDebug(grabber_audio_flow) << "Instance [" << hyperionInd << "] - Adding audio grabber" << _grabberName;
			GRABBER_AUDIO_CLIENTS.insert(hyperionInd, _grabberName);
			if (GRABBER_AUDIO_CLIENTS.size() == 1)
			{
				// If this is the first client, start the grabber
				qCDebug(grabber_audio_flow) << "Instance [" << hyperionInd << "] - First instance available for audio grabber";
				start();
			}
		}
		else
		{
			// If the audio grabber is released
			qCDebug(grabber_audio_flow) << "Removing audio grabber" << GRABBER_AUDIO_CLIENTS[hyperionInd] << "from instance [" << hyperionInd << "]";
			GRABBER_AUDIO_CLIENTS.remove(hyperionInd);
			if (GRABBER_AUDIO_CLIENTS.empty() || !getAudioGrabberState())
			{
				// If there are no more clients or the grabber is disabled, stop it
				Debug(_log, "Stop audio grabber %s, as no instance is listing any longer", QSTRING_CSTR(_grabberName));
				stop();
			}
		}
	}
}

void GrabberWrapper::handleSourceRequest(hyperion::Components component, int hyperionInd, bool listen)
{
	// This method handles requests to enable or disable a grabber for a specific Hyperion instance.
	// It manages the lifecycle of the grabber, starting it when the first instance requests it
	// and stopping it when the last instance releases it.
	handleSourceRequestScreen(component, hyperionInd, listen);
	handleSourceRequestVideo(component, hyperionInd, listen);
	handleSourceRequestAudio(component, hyperionInd, listen);
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
