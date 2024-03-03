
#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <QThread>

#include <utils/Components.h>
#include <utils/JsonUtils.h>
#include <utils/Image.h>

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

// ssdp
#include <ssdp/SSDPHandler.h>

// settings
#include <hyperion/SettingsManager.h>

// AuthManager
#include <hyperion/AuthManager.h>

// InstanceManager Hyperion
#include <hyperion/HyperionIManager.h>

// NetOrigin checks
#include <utils/NetOrigin.h>

#if defined(ENABLE_EFFECTENGINE)
// Init Python
#include <python/PythonInit.h>

// EffectFileHandler
#include <effectengine/EffectFileHandler.h>
#endif

#ifdef ENABLE_CEC
#include <cec/CECHandler.h>
#endif

HyperionDaemon* HyperionDaemon::daemon = nullptr;

HyperionDaemon::HyperionDaemon(const QString& rootPath, QObject* parent, bool logLvlOverwrite, bool readonlyMode)
	: QObject(parent), _log(Logger::getInstance("DAEMON"))
	, _instanceManager(new HyperionIManager(rootPath, this, readonlyMode))
	, _settingsManager(new SettingsManager(GLOABL_INSTANCE_ID, this, readonlyMode)) // init settings, this settingsManager accesses global settings which are independent from instances
	#if defined(ENABLE_EFFECTENGINE)
	, _pyInit(new PythonInit())
	#endif
	, _authManager(new AuthManager(this, readonlyMode))
	, _netOrigin(new NetOrigin(this))
	, _currVideoMode(VideoMode::VIDEO_2D)
{
	HyperionDaemon::daemon = this;

	// Register metas for thread queued connection
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");
	qRegisterMetaType<settings::type>("settings::type");
	qRegisterMetaType<VideoMode>("VideoMode");
	qRegisterMetaType<QMap<quint8, QJsonObject>>("QMap<quint8,QJsonObject>");
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");

	// set inital log lvl if the loglvl wasn't overwritten by arg
	if (!logLvlOverwrite)
	{
		handleSettingsUpdate(settings::LOGGER, getSetting(settings::LOGGER));
	}

#ifdef ENABLE_MDNS
	//Create MdnsBrowser singleton in main tread to ensure thread affinity during destruction
	MdnsBrowser::getInstance();
#endif

#if defined(ENABLE_EFFECTENGINE)
	// init EffectFileHandler
	EffectFileHandler* efh = new EffectFileHandler(rootPath, getSetting(settings::EFFECTS), this);
	connect(this, &HyperionDaemon::settingsChanged, efh, &EffectFileHandler::handleSettingsUpdate);
#endif

	// spawn all Hyperion instances (non blocking)
	_instanceManager->startAll();

	//Cleaning up Hyperion before quit
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &HyperionDaemon::stoppServices);

	//Handle services dependent on the on first instance's availability
	connect(_instanceManager.get(), &HyperionIManager::instanceStateChanged, this, &HyperionDaemon::handleInstanceStateChange);

	// pipe settings changes from HyperionIManager to Daemon
	connect(_instanceManager.get(), &HyperionIManager::settingsChanged, this, &HyperionDaemon::settingsChanged);

	// listen for setting changes of framegrabber and v4l2
	connect(this, &HyperionDaemon::settingsChanged, this, &HyperionDaemon::handleSettingsUpdate);

	// forward videoModes from HyperionIManager to Daemon evaluation
	connect(_instanceManager.get(), &HyperionIManager::requestVideoMode, this, &HyperionDaemon::setVideoMode);
	// return videoMode changes from Daemon to HyperionIManager
	connect(this, &HyperionDaemon::videoMode, _instanceManager.get(), &HyperionIManager::newVideoMode);

	createNetworkServices();
}

HyperionDaemon::~HyperionDaemon()
{
#if defined(ENABLE_EFFECTENGINE)
	delete _pyInit;
#endif
	Info(_log, "Hyperion daemon stopped");
}

void HyperionDaemon::handleInstanceStateChange(InstanceState state, quint8 instance)
{
	if (instance == 0)
	{
		switch (state)
		{
		case InstanceState::H_STARTED:

			startNetworkServices();
			startEventServices();
			startGrabberServices();

			break;

		case InstanceState::H_STOPPED:
		case InstanceState::H_CREATED:
		case InstanceState::H_ON_STOP:
		case InstanceState::H_DELETED:
			break;

		default:
			qWarning() << "HyperionDaemon::handleInstanceStateChange - Unhandled state:" << static_cast<int>(state);
			break;
		}
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

void HyperionDaemon::stoppServices()
{
	Info(_log, "Stopping Hyperion services.");

	QObject::disconnect(_instanceManager.get(), nullptr);
	QObject::disconnect(this, nullptr);

	stopGrabberServices();
	stopEventServices();
	stopNetworkServices();

	_instanceManager->stopAll();
}

void HyperionDaemon::createNetworkServices()
{
	// connect and apply settings for AuthManager
	connect(this, &HyperionDaemon::settingsChanged, _authManager.get(), &AuthManager::handleSettingsUpdate);
	_authManager->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

	// connect and apply settings for NetOrigin
	connect(this, &HyperionDaemon::settingsChanged, _netOrigin.get(), &NetOrigin::handleSettingsUpdate);
	_netOrigin->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

#ifdef ENABLE_MDNS
	// Create mDNS-Provider in own thread to allow publishing other services
	_mDNSProvider.reset(new MdnsProvider());
	QThread* mDnsThread = new QThread();
	mDnsThread->setObjectName("mDNSProviderThread");
	connect(mDnsThread, &QThread::started, _mDNSProvider.get(), &MdnsProvider::init);
	connect(mDnsThread, &QThread::finished, _mDNSProvider.get(), &MdnsProvider::deleteLater);
	_mDNSProvider->moveToThread(mDnsThread);
#endif

	// Create JSON server in own thread
	_jsonServer.reset(new JsonServer(getSetting(settings::JSONSERVER)));
	QThread* jsonThread = new QThread();
	jsonThread->setObjectName("JSONServerThread");
	connect(jsonThread, &QThread::started, _jsonServer.get(), &JsonServer::initServer);
	connect(jsonThread, &QThread::finished, _jsonServer.get(), &JsonServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _jsonServer.get(), &JsonServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
	connect(_jsonServer.get(), &JsonServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif
	_jsonServer->moveToThread(jsonThread);

	// Create Webserver in own thread
	_webserver.reset(new WebServer(getSetting(settings::WEBSERVER), false));
	QThread* wsThread = new QThread();
	wsThread->setObjectName("WebServerThread");
	connect(wsThread, &QThread::started, _webserver.get(), &WebServer::initServer);
	connect(wsThread, &QThread::finished, _webserver.get(), &WebServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _webserver.get(), &WebServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
	connect(_webserver.get(), &WebServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif
	_webserver->moveToThread(wsThread);

	// Create SSL Webserver in own thread
	_sslWebserver.reset(new WebServer(getSetting(settings::WEBSERVER), true));
	QThread* sslWsThread = new QThread();
	sslWsThread->setObjectName("SSLWebServerThread");
	connect(sslWsThread, &QThread::started, _sslWebserver.get(), &WebServer::initServer);
	connect(sslWsThread, &QThread::finished, _sslWebserver.get(), &WebServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _sslWebserver.get(), &WebServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
	connect(_sslWebserver.get(), &WebServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif
	_sslWebserver->moveToThread(sslWsThread);

	// Create SSDP server
	_ssdp.reset(new SSDPHandler(_webserver.get(),
			getSetting(settings::FLATBUFSERVER).object()["port"].toInt(),
			getSetting(settings::PROTOSERVER).object()["port"].toInt(),
			getSetting(settings::JSONSERVER).object()["port"].toInt(),
			getSetting(settings::WEBSERVER).object()["sslPort"].toInt(),
			getSetting(settings::GENERAL).object()["name"].toString()));
	QThread* ssdpThread = new QThread();
	ssdpThread->setObjectName("SSDPThread");
	connect(ssdpThread, &QThread::started, _ssdp.get(), &SSDPHandler::initServer);
	connect(ssdpThread, &QThread::finished, _ssdp.get(), &SSDPHandler::deleteLater);
	connect(_webserver.get(), &WebServer::stateChange, _ssdp.get(), &SSDPHandler::handleWebServerStateChange);
	connect(this, &HyperionDaemon::settingsChanged, _ssdp.get(), &SSDPHandler::handleSettingsUpdate);
	_ssdp->moveToThread(ssdpThread);

#if defined(ENABLE_FLATBUF_SERVER)
	// Create FlatBuffer server in thread
	_flatBufferServer.reset(new FlatBufferServer(getSetting(settings::FLATBUFSERVER)));
	QThread* fbThread = new QThread();
	fbThread->setObjectName("FlatBufferServerThread");
	connect(fbThread, &QThread::started, _flatBufferServer.get(), &FlatBufferServer::initServer);
	connect(fbThread, &QThread::finished, _flatBufferServer.get(), &FlatBufferServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _flatBufferServer.get(), &FlatBufferServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
	connect(_flatBufferServer.get(), &FlatBufferServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif
	_flatBufferServer->moveToThread(fbThread);
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	// Create Proto server in thread
	_protoServer.reset(new ProtoServer(getSetting(settings::PROTOSERVER)));
	QThread* pThread = new QThread();
	pThread->setObjectName("ProtoServerThread");
	connect(pThread, &QThread::started, _protoServer.get(), &ProtoServer::initServer);
	connect(pThread, &QThread::finished, _protoServer.get(), &ProtoServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _protoServer.get(), &ProtoServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
	connect(_protoServer.get(), &ProtoServer::publishService, _mDNSProvider.get(), &MdnsProvider::publishService);
#endif
	_protoServer->moveToThread(pThread);
#endif
}

void HyperionDaemon::startNetworkServices()
{
	_jsonServer->thread()->start();
	_webserver->thread()->start();
	_sslWebserver->thread()->start();

	_mDNSProvider->thread()->start();
	_ssdp->thread()->start();

#if defined(ENABLE_FLATBUF_SERVER)
	_flatBufferServer->thread()->start();
#endif
#if defined(ENABLE_PROTOBUF_SERVER)
	_protoServer->thread()->start();
#endif
}

void HyperionDaemon::stopNetworkServices()
{
#if defined(ENABLE_PROTOBUF_SERVER)
	_protoServer.reset(nullptr);
#endif
#if defined(ENABLE_FLATBUF_SERVER)
	_flatBufferServer.reset(nullptr);
#endif

	_mDNSProvider.reset(nullptr);
	_ssdp.reset(nullptr);

	_sslWebserver.reset(nullptr);
	_webserver.reset(nullptr);
	_jsonServer.reset(nullptr);
}

void HyperionDaemon::startEventServices()
{
	_eventHandler->getInstance();

	_eventScheduler.reset(new EventScheduler());
	_eventScheduler->handleSettingsUpdate(settings::SCHEDEVENTS, getSetting(settings::SCHEDEVENTS));
	connect(this, &HyperionDaemon::settingsChanged, _eventScheduler.get(), &EventScheduler::handleSettingsUpdate);

	_osEventHandler.reset(new OsEventHandler());
	_osEventHandler->handleSettingsUpdate(settings::OSEVENTS, getSetting(settings::OSEVENTS));
	connect(this, &HyperionDaemon::settingsChanged, _osEventHandler.get(), &OsEventHandler::handleSettingsUpdate);

#if defined(ENABLE_CEC)
	_cecHandler.reset(new CECHandler(getSetting(settings::CECEVENTS)));
	QThread* cecHandlerThread = new QThread();
	cecHandlerThread->setObjectName("CECThread");
	connect(cecHandlerThread, &QThread::started, _cecHandler.get(), &CECHandler::start);
	connect(cecHandlerThread, &QThread::finished, _cecHandler.get(), &CECHandler::stop);
	connect(cecHandlerThread, &QThread::finished, _cecHandler.get(), &CECHandler::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _cecHandler.get(), &CECHandler::handleSettingsUpdate);
	Info(_log, "CEC event handler created");

	_cecHandler->moveToThread(cecHandlerThread);
	cecHandlerThread->start();
#else
	Debug(_log, "The CEC handler is not supported on this platform");
#endif
}

void HyperionDaemon::stopEventServices()
{
#if defined(ENABLE_CEC)
	_cecHandler.reset(nullptr);
#endif
	_osEventHandler.reset(nullptr);
	_eventScheduler.reset(nullptr);
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

void HyperionDaemon::stopGrabberServices()
{
	_screenGrabber.reset();
	_videoGrabber.reset();
	_audioGrabber.reset();
}

void HyperionDaemon::handleSettingsUpdate(settings::type settingsType, const QJsonDocument& config)
{
	if (settingsType == settings::LOGGER)
	{
		const QJsonObject& logConfig = config.object();

		std::string level = logConfig["level"].toString("warn").toStdString(); // silent warn verbose debug
		if (level == "silent")
		{
			Logger::setLogLevel(Logger::OFF);
		}
		else if (level == "warn")
		{
			Logger::setLogLevel(Logger::LogLevel::WARNING);
		}
		else if (level == "verbose")
		{
			Logger::setLogLevel(Logger::INFO);
		}
		else if (level == "debug")
		{
			Logger::setLogLevel(Logger::DEBUG);
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
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB) && !defined(ENABLE_X11) && !defined(ENABLE_XCB) && !defined(ENABLE_AMLOGIC) && !defined(ENABLE_QT) && !defined(ENABLE_DX)
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

			QString amlDevice("/dev/amvideocap0");
			if (!QFile::exists(amlDevice))
			{
				Error(_log, "grabber device '%s' for type amlogic not found!", QSTRING_CSTR(amlDevice));
			}
		}
		else
		{
			// x11 -> if DISPLAY is set
			QByteArray envDisplay = qgetenv("DISPLAY");
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
