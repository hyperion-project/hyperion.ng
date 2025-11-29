#include <string>
#include <vector>

#include <QCoreApplication>
#include <QtGlobal>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <QThread>
#include <QByteArray>
#include <QMap>
#include <QMetaType>

#include "utils/Logger.h"
#include "db/SettingsTable.h"
#include "events/EventScheduler.h"
#include "events/OsEventHandler.h"
#include <events/EventHandler.h>
#include <utils/Components.h>
#include <utils/Image.h>
#include <utils/settings.h>
#include "utils/ColorRgb.h"
#include "utils/VideoMode.h"
#include "utils/global_defines.h"
#include <utils/GlobalSignals.h>
#include <utils/ThreadUtils.h>

#include <HyperionConfig.h> // Required to determine the cmake options

// mDNS
#ifdef ENABLE_MDNS
#include <mdns/MdnsProvider.h>
#include <mdns/MdnsBrowser.h>
#endif

#include <jsonserver/JsonServer.h>
#include <webserver/WebServer.h>
#include "hyperiond.h"

// Flatbuffer Server
#ifdef ENABLE_FLATBUF_SERVER
#include <flatbufserver/FlatBufferServer.h>
#endif

// Protobuffer Server
#ifdef ENABLE_PROTOBUF_SERVER
#include <protoserver/ProtoServer.h>
#endif

// Message Forwarder
#if defined(ENABLE_FORWARDER)
#include <forwarder/MessageForwarder.h>
#endif

// ssdp
#include <ssdp/SSDPHandler.h>

// settings
#include <hyperion/SettingsManager.h>

// AuthManager
#include <hyperion/AuthManager.h>
// EffectFileHandler singleton
#if defined(ENABLE_EFFECTENGINE)
#include <effectengine/EffectFileHandler.h>
#endif
// NetOrigin singleton
#include <utils/NetOrigin.h>

// InstanceManager Hyperion
#include <hyperion/HyperionIManager.h>
 
#if defined(ENABLE_EFFECTENGINE)
// Init Python
#include <python/PythonInit.h>
#endif

#ifdef ENABLE_CEC
#include <cec/CECHandler.h>
#endif

namespace {
	const GlobalSignals * const ensureGlobalSignalsInitialized = GlobalSignals::getInstance();
}

HyperionDaemon* HyperionDaemon::daemon = nullptr;

HyperionDaemon::HyperionDaemon(const QString& rootPath, QObject* parent, bool logLvlOverwrite)
	: QObject(parent), _log(Logger::getInstance("DAEMON"))
	, _settingsManager(new SettingsManager(NO_INSTANCE_ID, this)) // init settings, this settingsManager accesses global settings which are independent from instances
	#if defined(ENABLE_EFFECTENGINE)
	, _pyInit(new PythonInit())
	#endif
	, _currVideoMode(VideoMode::VIDEO_2D)
{
	TRACK_SCOPE();
	HyperionDaemon::daemon = this;

	// Register metas for thread queued connection
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");
	qRegisterMetaType<settings::type>("settings::type");
	qRegisterMetaType<VideoMode>("VideoMode");
	qRegisterMetaType<QMap<quint8, QJsonObject>>("QMap<quint8,QJsonObject>");
	qRegisterMetaType<QVector<ColorRgb>>("QVector<ColorRgb>");

	// set inital log lvl if the loglvl wasn't overwritten by arg
	if (!logLvlOverwrite)
	{
		handleSettingsUpdate(settings::LOGGER, getSetting(settings::LOGGER));
	}

	AuthManager::createInstance(this);
	if (auto auth = AuthManager::getInstanceWeak().toStrongRef())
	{
		connect(this, &HyperionDaemon::settingsChanged, auth.get(), &AuthManager::handleSettingsUpdate);
		auth->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));
	}	

#if defined(ENABLE_EFFECTENGINE)
	EffectFileHandler::createInstance(rootPath, getSetting(settings::EFFECTS), this);
	if (auto eff = EffectFileHandler::getInstanceWeak().toStrongRef())
	{
		connect(this, &HyperionDaemon::settingsChanged, eff.get(), &EffectFileHandler::handleSettingsUpdate);
	}
#endif

	//Cleaning up Hyperion before quit
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &HyperionDaemon::stopServices);

	// Create instance manager singleton & spawn all Hyperion instances (non blocking)
	HyperionIManager::createInstance(this);
	_instanceManagerWeak = HyperionIManager::getInstanceWeak();
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		mgr->startAll();

		//Handle services dependent on the first instance's availability
		connect(mgr.get(), &HyperionIManager::instanceStateChanged, this, &HyperionDaemon::handleInstanceStateChange);
		connect(mgr.get(), &HyperionIManager::settingsChanged, this, &HyperionDaemon::settingsChanged);

		// forward videoModes from HyperionIManager to Daemon evaluation
		connect(mgr.get(), &HyperionIManager::requestVideoMode, this, &HyperionDaemon::setVideoMode);
		connect(this, &HyperionDaemon::videoMode, mgr.get(), &HyperionIManager::newVideoMode);
	}

	// listen for setting changes of framegrabber and v4l2
	connect(this, &HyperionDaemon::settingsChanged, this, &HyperionDaemon::handleSettingsUpdate);

	startEventServices();
	
	createNetworkServices();
	createNetworkInputCaptureServices();
	createNetworkOutputServices();

	startNetworkServices();
	startGrabberServices();

	startNetworkOutputServices();
}

HyperionDaemon::~HyperionDaemon()
{
	TRACK_SCOPE();
	Info(_log, "Hyperion daemon stopped");
}

void HyperionDaemon::handleInstanceStateChange(InstanceState state, quint8 instance)
{
	switch (state)
	{
	case InstanceState::H_STARTING:
	break;
	case InstanceState::H_STARTED:
	{
		restartGrabberServices();
		startNetworkInputCaptureServices();
#if defined(ENABLE_FORWARDER)
		QMetaObject::invokeMethod(_messageForwarder.get(),
			[this, instance]() {
				if (_messageForwarder->connectToInstance(instance))
				{
					_messageForwarder->start();
				}
			},
			Qt::QueuedConnection);
#endif
	}
	break;
	case InstanceState::H_STOPPED:
#if defined(ENABLE_FORWARDER)
		QMetaObject::invokeMethod(_messageForwarder.get(),
			[this, instance]() { _messageForwarder->disconnectFromInstance(instance); },
			Qt::QueuedConnection);
#endif

		if (auto mgr = _instanceManagerWeak.toStrongRef())
		{
			if(mgr->getRunningInstanceIdx().empty())
			{
				stopGrabberServices();
#if defined(ENABLE_FORWARDER)
				QMetaObject::invokeMethod(_messageForwarder.get(), &MessageForwarder::stop, Qt::QueuedConnection);
#endif
				stopNetworkInputCaptureServices();
			}
		}
		break;
	case InstanceState::H_CREATED:
	break;
	case InstanceState::H_ON_STOP:
	break;
	case InstanceState::H_DELETED:
	break;

	default:
		qWarning() << "HyperionDaemon::handleInstanceStateChange - Unhandled state:" << static_cast<int>(state);
	break;
	}

}

void HyperionDaemon::setVideoMode(VideoMode mode)
{
	if (_currVideoMode != mode)
	{
		_currVideoMode = mode;
		emit videoMode(mode);
	}
}

QJsonDocument HyperionDaemon::getSetting(settings::type type) const
{
	return _settingsManager->getSetting(type);
}

void HyperionDaemon::stopServices()
{
	Info(_log, "Stopping Hyperion services...");

	// Stop event services first to inform clients about shutdown and prevent further communication via JSON API
	stopEventServices();
	
	stopGrabberServices();

	// Ensure that all Instances and their threads are stopped
	QEventLoop loopInstances;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		QObject::connect(mgr.get(), &HyperionIManager::areAllInstancesStopped, &loopInstances, &QEventLoop::quit);
		mgr->stopAll();
	}
	loopInstances.exec();

	stopNetworkOutputServices();
	stopNetworkServices();

	HyperionIManager::destroyInstance();

	// Destroy service singletons

#if defined(ENABLE_EFFECTENGINE)
	EffectFileHandler::destroyInstance();
	delete _pyInit;
#endif
	AuthManager::destroyInstance();
	NetOrigin::destroyInstance();
}

void HyperionDaemon::createNetworkServices()
{
	// Ensure NetOrigin singleton exists before servers use it
	NetOrigin::createInstance(this);

	// AuthManager already created in constructor; nothing further needed here

#ifdef ENABLE_MDNS
	// Create mDNS-Provider and mDNS-Browser in own thread
	_mDnsThread.reset(new QThread());
	_mDnsThread->setObjectName("mDNSThread");
	_mDNSProvider.reset(new MdnsProvider());
	_mDNSProvider->moveToThread(_mDnsThread.get());
	connect(_mDnsThread.get(), &QThread::started, _mDNSProvider.get(), &MdnsProvider::init);

	MdnsBrowser::getInstance(_mDnsThread.get());
#endif

	// Create SSDP server
	_ssdpHandlerThread.reset(new QThread());
	_ssdpHandlerThread->setObjectName("SSDPThread");
	_ssdpHandler.reset(new SSDPHandler());
	_ssdpHandler->moveToThread(_ssdpHandlerThread.get());
	connect(_ssdpHandlerThread.get(), &QThread::started, _ssdpHandler.get(), &SSDPHandler::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _ssdpHandler.get(), &SSDPHandler::handleSettingsUpdate);
	_ssdpHandler->handleSettingsUpdate(settings::GENERAL, _settingsManager->getSetting(settings::GENERAL));

	// Create JSON server in own thread
	_jsonServerThread.reset(new QThread());
	_jsonServerThread->setObjectName("JSONServerThread");
	_jsonServer.reset(new JsonServer(getSetting(settings::JSONSERVER)));
	_jsonServer->moveToThread(_jsonServerThread.get());

	connect(_jsonServer.get(), &JsonServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
#ifdef ENABLE_MDNS
	connect(_jsonServer.get(), &JsonServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_jsonServerThread.get(), &QThread::started, _jsonServer.get(), &JsonServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _jsonServer.get(), &JsonServer::handleSettingsUpdate);

	// Create Webserver in own thread
	_webServerThread.reset(new QThread());
	_webServerThread->setObjectName("WebServerThread");
	_webServer.reset(new WebServer(getSetting(settings::WEBSERVER), false));
	_webServer->moveToThread(_webServerThread.get());

	connect(_webServer.get(), &WebServer::stateChange, _ssdpHandler.get(), &SSDPHandler::onStateChange);
	connect(_webServer.get(), &WebServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
	connect(_ssdpHandler.get(), &SSDPHandler::descriptionUpdated, _webServer.get(), &WebServer::onSsdpDescriptionUpdated);
#ifdef ENABLE_MDNS
	connect(_webServer.get(), &WebServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_webServerThread.get(), &QThread::started, _webServer.get(), &WebServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _webServer.get(), &WebServer::handleSettingsUpdate);

	// Create SSL Webserver in own thread
	_sslWebServerThread.reset(new QThread());
	_sslWebServerThread->setObjectName("SSLWebServerThread");
	_sslWebServer.reset(new WebServer(getSetting(settings::WEBSERVER), true));
	_sslWebServer->moveToThread(_sslWebServerThread.get());

	connect(_sslWebServer.get(), &WebServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
	connect(_ssdpHandler.get(), &SSDPHandler::descriptionUpdated, _sslWebServer.get(), &WebServer::onSsdpDescriptionUpdated);
#ifdef ENABLE_MDNS
	connect(_sslWebServer.get(), &WebServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_sslWebServerThread.get(), &QThread::started, _sslWebServer.get(), &WebServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _sslWebServer.get(), &WebServer::handleSettingsUpdate);
}

void HyperionDaemon::startNetworkServices()
{
	//Start JSON-Server only after WebServery were started to avoid port mismatches
	connect(_sslWebServerThread.get(), &QThread::started, [=]() {
		_jsonServerThread->start();
		});

	_webServerThread->start();
	_sslWebServerThread->start();

#if defined(ENABLE_MDNS)
	_mDnsThread->start();
#endif
	_ssdpHandlerThread->start();
}

void HyperionDaemon::stopNetworkServices()
{
#if defined(ENABLE_MDNS)
	if (_mDnsThread->isRunning() && MdnsBrowser::getInstance().get() && _mDNSProvider.get())
	{
		QObject::connect(MdnsBrowser::getInstance().get(), &MdnsBrowser::isStopped,
			_mDNSProvider.get(), &MdnsProvider::stop,
			Qt::QueuedConnection);

		QObject::connect(_mDNSProvider.get(), &MdnsProvider::isStopped,
			_mDnsThread.get(), &QThread::quit,
			Qt::DirectConnection);

		QMetaObject::invokeMethod(MdnsBrowser::getInstance().get(), &MdnsBrowser::stop, Qt::QueuedConnection);

		if (!_mDnsThread->wait(5000)) {
			qWarning() << "mDNS thread failed to exit gracefully!";
		}
	}
	MdnsBrowser::destroyInstance();
#endif

	safeShutdownThread(
		_webServer.get(),
		_webServerThread.get(),
		&WebServer::isStopped,
		&WebServer::stop,
		5000 // 5 second timeout
	);

	safeShutdownThread(
		_sslWebServer.get(),
		_sslWebServerThread.get(),
		&WebServer::isStopped,
		&WebServer::stop
	);

	safeShutdownThread(
		_jsonServer.get(),
		_jsonServerThread.get(),
		&JsonServer::isStopped,
		&JsonServer::stop,
		5000 // 5 second timeout
	);

	safeShutdownThread(
		_ssdpHandler.get(),
		_ssdpHandlerThread.get(),
		&SSDPHandler::isStopped,
		&SSDPHandler::stop
	);
}

void HyperionDaemon::createNetworkInputCaptureServices()
{
#if defined(ENABLE_FLATBUF_SERVER)
	// Create FlatBuffer server in thread
	_flatBufferServerThread.reset(new QThread());
	_flatBufferServerThread->setObjectName("FlatBufferServerThread");
	_flatBufferServer.reset(new FlatBufferServer(getSetting(settings::FLATBUFSERVER)));
	_flatBufferServer->moveToThread(_flatBufferServerThread.get());

	connect(_flatBufferServer.get(), &FlatBufferServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
#ifdef ENABLE_MDNS
	connect(_flatBufferServer.get(), &FlatBufferServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_flatBufferServerThread.get(), &QThread::started, _flatBufferServer.get(), &FlatBufferServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _flatBufferServer.get(), &FlatBufferServer::handleSettingsUpdate);
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	// Create Proto server in thread
	_protoServerThread.reset(new QThread());
	_protoServerThread->setObjectName("ProtoServerThread");
	_protoServer.reset(new ProtoServer(getSetting(settings::PROTOSERVER)));
	_protoServer->moveToThread(_protoServerThread.get());

	connect(_protoServer.get(), &ProtoServer::publishService, _ssdpHandler.get(), &SSDPHandler::onPortChanged);
#ifdef ENABLE_MDNS
	connect(_protoServer.get(), &ProtoServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif

	connect(_protoServerThread.get(), &QThread::started, _protoServer.get(), &ProtoServer::initServer);
	connect(this, &HyperionDaemon::settingsChanged, _protoServer.get(), &ProtoServer::handleSettingsUpdate);
#endif
}

void HyperionDaemon::startNetworkInputCaptureServices()
{
#if defined(ENABLE_FLATBUF_SERVER)
	if (!_flatBufferServerThread->isRunning())
	{
		_flatBufferServerThread->start();
	}
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
	if (!_protoServerThread->isRunning())
	{
		_protoServerThread->start();
	}
#endif
}

void HyperionDaemon::stopNetworkInputCaptureServices()
{
#if defined(ENABLE_PROTOBUF_SERVER)
	safeShutdownThread(
		_protoServer.get(),
		_protoServerThread.get(),
		&ProtoServer::isStopped,
		&ProtoServer::stop
	);
#endif

#if defined(ENABLE_FLATBUF_SERVER)
	safeShutdownThread(
		_flatBufferServer.get(),
		_flatBufferServerThread.get(),
		&FlatBufferServer::isStopped,
		&FlatBufferServer::stop
	);
#endif
}

void HyperionDaemon::createNetworkOutputServices()
{
#if defined(ENABLE_FORWARDER)
	// Create Message Forwarder in thread
	_messageForwarderThread.reset(new QThread());
	_messageForwarderThread->setObjectName("MessageForwarderThread");
	_messageForwarder.reset(new MessageForwarder(getSetting(settings::NETFORWARD)));
	_messageForwarder->moveToThread(_messageForwarderThread.get());

	connect(_messageForwarderThread.get(), &QThread::started, _messageForwarder.get(), &MessageForwarder::init);
	connect(this, &HyperionDaemon::settingsChanged, _messageForwarder.get(), &MessageForwarder::handleSettingsUpdate);
#endif
}

void HyperionDaemon::startNetworkOutputServices()
{
#if defined(ENABLE_FORWARDER)
	if (!_messageForwarderThread->isRunning())
	{
		_messageForwarderThread->start();
	}
#endif
}

void HyperionDaemon::stopNetworkOutputServices()
{
#if defined(ENABLE_FORWARDER)
	safeShutdownThread(
		_messageForwarder.get(),
		_messageForwarderThread.get(),
		&MessageForwarder::stopped,
		&MessageForwarder::stop
	);
#endif
}

void HyperionDaemon::startEventServices()
{
	// Ensure EventHandler singleton is created early
	EventHandler::getInstance();

	_eventScheduler.reset(new EventScheduler());
	_eventScheduler->handleSettingsUpdate(settings::SCHEDEVENTS, getSetting(settings::SCHEDEVENTS));
	connect(this, &HyperionDaemon::settingsChanged, _eventScheduler.get(), &EventScheduler::handleSettingsUpdate);

	_osEventHandler.reset(new OsEventHandler());
	_osEventHandler->handleSettingsUpdate(settings::OSEVENTS, getSetting(settings::OSEVENTS));
	connect(this, &HyperionDaemon::settingsChanged, _osEventHandler.get(), &OsEventHandler::handleSettingsUpdate);

#if defined(ENABLE_CEC)
	_cecHandlerThread.reset(new QThread());
	_cecHandlerThread->setObjectName("CECThread");
	_cecHandler.reset(new CECHandler(getSetting(settings::CECEVENTS)));
	_cecHandler->moveToThread(_cecHandlerThread.get());
	connect(_cecHandlerThread.get(), &QThread::started, _cecHandler.get(), &CECHandler::start);
	connect(this, &HyperionDaemon::settingsChanged, _cecHandler.get(), &CECHandler::handleSettingsUpdate);
	Info(_log, "CEC event handler created");
	_cecHandlerThread->start();
#else
	Debug(_log, "The CEC handler is not supported on this platform");
#endif
}

void HyperionDaemon::stopEventServices()
{
#if defined(ENABLE_CEC)
	safeShutdownThread(
		_cecHandler.get(),
		_cecHandlerThread.get(),
		&CECHandler::isStopped,
		&CECHandler::stop
	);
#endif

	_osEventHandler.reset(nullptr);
	_eventScheduler.reset(nullptr);

	// Destroy EventHandler singleton explicitly during shutdown
	EventHandler::destroyInstance();
}

void HyperionDaemon::startGrabberServices()
{
	// start screen capture
	handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));

	// start video capture (v4l2 && media foundation)
	handleSettingsUpdate(settings::V4L2, getSetting(settings::V4L2));

	// start audio capture
	handleSettingsUpdate(settings::AUDIO, getSetting(settings::AUDIO));
}

void HyperionDaemon::restartGrabberServices()
{
	_screenGrabber->start();
	_videoGrabber->start();
	_audioGrabber->start();
}

void HyperionDaemon::stopGrabberServices()
{
	_screenGrabber->stop();
	_videoGrabber->stop();
	_audioGrabber->stop();
}

void HyperionDaemon::handleSettingsUpdate(settings::type settingsType, const QJsonDocument& config)
{
	if (settingsType == settings::LOGGER)
	{
		const QJsonObject& logConfig = config.object();

		std::string const level = logConfig["level"].toString("warn").toStdString(); // silent warn verbose debug
		if (level == "silent")
		{
			Logger::setLogLevel(Logger::LogLevel::Off);
		}
		else if (level == "off")
		{
			Logger::setLogLevel(Logger::LogLevel::Off);
		}
		else if (level == "warn")
		{
			Logger::setLogLevel(Logger::LogLevel::Warning);
		}
		else if (level == "verbose")
		{
			Logger::setLogLevel(Logger::LogLevel::Info);
		}
		else if (level == "debug")
		{
			Logger::setLogLevel(Logger::LogLevel::Debug);
		}
	}

	if (settingsType == settings::SYSTEMCAPTURE)
	{
		updateScreenGrabbers(config);
	}
	else if (settingsType == settings::V4L2)
	{
		const QJsonObject& grabberConfig = config.object();
		updateVideoGrabbers(grabberConfig);
	}
	else if (settingsType == settings::AUDIO)
	{
		const QJsonObject& grabberConfig = config.object();
		updateAudioGrabbers(grabberConfig);
	}
}

void HyperionDaemon::updateScreenGrabbers(const QJsonDocument& grabberConfig)
{
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB) && !defined(ENABLE_X11) && !defined(ENABLE_XCB) && !defined(ENABLE_AMLOGIC) && !defined(ENABLE_QT) && !defined(ENABLE_DX) && !defined(ENABLE_DDA) && !defined(ENABLE_DRM)
	Info(_log, "No screen capture supported on this platform");
	return;
#endif

#ifdef ENABLE_OSX
	QString type = grabberConfig["device"].toString("osx");
#else
	QString type = grabberConfig["device"].toString("auto");
#endif

	if (type == "auto")
	{
		type = evalScreenGrabberType();
	}

	if (_prevType != type)
	{
		//Stop previous grabber
		_screenGrabber.reset();

		if (type == "auto")
		{
			Error(_log, "The automatic screen grabber type evaluation did not work.");
			return;
		}
#ifdef ENABLE_AMLOGIC
		else if (type == "amlogic")
		{
			startGrabber<AmlogicWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_DISPMANX
		else if (type == "dispmanx")
		{
			startGrabber<DispmanxWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_DX
		else if (type == "dx")
		{
			startGrabber<DirectXWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_DDA
		else if (type == "dda")
		{
			startGrabber<DDAWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_DRM
		else if (type == "drm")
		{
			startGrabber<DRMWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_FB
		else if (type == "framebuffer")
		{
			startGrabber<FramebufferWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_OSX
		else if (type == "osx")
		{

			startGrabber<OsxWrapper>(_screenGrabber, grabberConfig);

		}
#endif
#ifdef ENABLE_QT
		else if (type == "qt")
		{
			startGrabber<QtWrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_X11
		else if (type == "x11")
		{
			startGrabber<X11Wrapper>(_screenGrabber, grabberConfig);
		}
#endif
#ifdef ENABLE_XCB
		else if (type == "xcb")
		{
			startGrabber<XcbWrapper>(_screenGrabber, grabberConfig);
		}
#endif
		else
		{
			Warning(_log, "The %s grabber is not enabled on this platform", QSTRING_CSTR(type));
			return;
		}
		_prevType = type;
	}
}

void HyperionDaemon::updateVideoGrabbers(const QJsonObject& /*grabberConfig*/)
{
#if defined(ENABLE_V4L2) || defined(ENABLE_MF)
	_videoGrabber.reset(new VideoWrapper());
	_videoGrabber->handleSettingsUpdate(settings::V4L2, getSetting(settings::V4L2));

#if defined(ENABLE_MF)
	Debug(_log, "Media Foundation grabber created");
#elif defined(ENABLE_V4L2)
	Debug(_log, "V4L2 grabber created");
#endif
	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _videoGrabber.get(), &VideoWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _videoGrabber.get(), &VideoWrapper::handleSettingsUpdate);

	Debug(_log, "Video grabber created");
#else
	Warning(_log, "No video capture supported on this platform");
#endif
}

void HyperionDaemon::updateAudioGrabbers(const QJsonObject& /*grabberConfig*/)
{
#ifdef ENABLE_AUDIO
	// Create Audio Grabber
	_audioGrabber.reset(new AudioWrapper());
	_audioGrabber->handleSettingsUpdate(settings::AUDIO, getSetting(settings::AUDIO));

	connect(this, &HyperionDaemon::settingsChanged, _audioGrabber.get(), &AudioWrapper::handleSettingsUpdate);

	Debug(_log, "Audio grabber created");
#else
	Warning(_log, "No audio capture supported on this platform");
#endif
}

QString HyperionDaemon::evalScreenGrabberType()
{
	QString type;

	// dispmanx -> on raspi
	if (QFile::exists("/dev/vchiq"))
	{
		type = "dispmanx";
	}
	// amlogic -> /dev/amvideo exists
	else
	{
		if (QFile::exists("/dev/amvideo"))
		{
			type = "amlogic";

			QString const amlDevice("/dev/amvideocap0");
			if (!QFile::exists(amlDevice))
			{
				Error(_log, "grabber device '%s' for type amlogic not found!", QSTRING_CSTR(amlDevice));
			}
		}
		else
		{
			// x11 -> if DISPLAY is set
			QByteArray const envDisplay = qgetenv("DISPLAY");
			if (!envDisplay.isEmpty())
			{
#if defined(ENABLE_X11)
				type = "x11";
#elif defined(ENABLE_XCB)
				type = "xcb";
#else
				type = "qt";
#endif
			}
			// qt -> if nothing other applies
			else
			{
				type = "qt";
			}
		}
	}

	return type;
}
