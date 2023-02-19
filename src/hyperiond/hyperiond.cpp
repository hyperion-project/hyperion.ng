#include <cassert>
#include <stdlib.h>

#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <cstdint>
#include <limits>
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
	, _authManager(new AuthManager(this, readonlyMode))
#ifdef ENABLE_MDNS
	, _mDNSProvider(nullptr)
#endif
	, _netOrigin(new NetOrigin(this))
#if defined(ENABLE_EFFECTENGINE)
	, _pyInit(new PythonInit())
#endif
	, _webserver(nullptr)
	, _sslWebserver(nullptr)
	, _jsonServer(nullptr)
	, _videoGrabber(nullptr)
	, _dispmanx(nullptr)
	, _x11Grabber(nullptr)
	, _xcbGrabber(nullptr)
	, _amlGrabber(nullptr)
	, _fbGrabber(nullptr)
	, _osxGrabber(nullptr)
	, _qtGrabber(nullptr)
	, _dxGrabber(nullptr)
	, _ssdp(nullptr)
	, _audioGrabber(nullptr)
#ifdef ENABLE_CEC
	, _cecHandler(nullptr)
#endif
	, _suspendHandler(nullptr)
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

	// init settings, this settingsManager accesses global settings which are independent from instances
	_settingsManager = new SettingsManager(GLOABL_INSTANCE_ID, this, readonlyMode);

	// set inital log lvl if the loglvl wasn't overwritten by arg
	if (!logLvlOverwrite)
	{
		handleSettingsUpdate(settings::LOGGER, getSetting(settings::LOGGER));
	}

	createCecHandler();

	//Create MdnsBrowser singleton in main tread to ensure thread affinity during destruction
#ifdef ENABLE_MDNS
	MdnsBrowser::getInstance();
#endif

#if defined(ENABLE_EFFECTENGINE)
	// init EffectFileHandler
	EffectFileHandler* efh = new EffectFileHandler(rootPath, getSetting(settings::EFFECTS), this);
	connect(this, &HyperionDaemon::settingsChanged, efh, &EffectFileHandler::handleSettingsUpdate);
#endif

	// connect and apply settings for AuthManager
	connect(this, &HyperionDaemon::settingsChanged, _authManager, &AuthManager::handleSettingsUpdate);
	_authManager->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

	// connect and apply settings for NetOrigin
	connect(this, &HyperionDaemon::settingsChanged, _netOrigin, &NetOrigin::handleSettingsUpdate);
	_netOrigin->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

	// spawn all Hyperion instances (non blocking)
	_instanceManager->startAll();

	//Cleaning up Hyperion before quit
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &HyperionDaemon::freeObjects);

	//Handle services dependent on the on first instance's availability
	connect(_instanceManager, &HyperionIManager::instanceStateChanged, this, &HyperionDaemon::handleInstanceStateChange);

	// pipe settings changes from HyperionIManager to Daemon
	connect(_instanceManager, &HyperionIManager::settingsChanged, this, &HyperionDaemon::settingsChanged);

	// listen for setting changes of framegrabber and v4l2
	connect(this, &HyperionDaemon::settingsChanged, this, &HyperionDaemon::handleSettingsUpdate);

	// forward videoModes from HyperionIManager to Daemon evaluation
	connect(_instanceManager, &HyperionIManager::requestVideoMode, this, &HyperionDaemon::setVideoMode);
	// return videoMode changes from Daemon to HyperionIManager
	connect(this, &HyperionDaemon::videoMode, _instanceManager, &HyperionIManager::newVideoMode);

	// ---- grabber -----
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB) && !defined(ENABLE_X11) && !defined(ENABLE_XCB) && !defined(ENABLE_AMLOGIC) && !defined(ENABLE_QT) && !defined(ENABLE_DX)
	Info(_log, "No platform capture supported on this platform");
#endif

	// init system capture (framegrabber)
	handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));

	// init v4l2 && media foundation capture
	handleSettingsUpdate(settings::V4L2, getSetting(settings::V4L2));

	// init audio capture
	handleSettingsUpdate(settings::AUDIO, getSetting(settings::AUDIO));

	// ---- network services -----
	startNetworkServices();

	_suspendHandler = new SuspendHandler();
}

HyperionDaemon::~HyperionDaemon()
{
	delete _settingsManager;
#if defined(ENABLE_EFFECTENGINE)
	delete _pyInit;
#endif
}

void HyperionDaemon::handleInstanceStateChange(InstanceState state, quint8 instance)
{
	switch (state)
	{
	case InstanceState::H_STARTED:

		if (instance == 0)
		{
			// Start Json server in own thread
			_jsonServer = new JsonServer(getSetting(settings::JSONSERVER));
			QThread* jsonThread = new QThread(this);
			jsonThread->setObjectName("JSONServerThread");
			_jsonServer->moveToThread(jsonThread);
			connect(jsonThread, &QThread::started, _jsonServer, &JsonServer::initServer);
			connect(jsonThread, &QThread::finished, _jsonServer, &JsonServer::deleteLater);
			connect(this, &HyperionDaemon::settingsChanged, _jsonServer, &JsonServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
			connect(_jsonServer, &JsonServer::publishService, _mDNSProvider, &MdnsProvider::publishService);
#endif
			jsonThread->start();

			// Start Webserver in own thread
			QThread* wsThread = new QThread(this);
			wsThread->setObjectName("WebServerThread");
			_webserver->moveToThread(wsThread);
			connect(wsThread, &QThread::started, _webserver, &WebServer::initServer);
			connect(wsThread, &QThread::finished, _webserver, &WebServer::deleteLater);
			connect(this, &HyperionDaemon::settingsChanged, _webserver, &WebServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
			connect(_webserver, &WebServer::publishService, _mDNSProvider, &MdnsProvider::publishService);
#endif
			wsThread->start();

			// Start SSL Webserver in own thread
			_sslWebserver = new WebServer(getSetting(settings::WEBSERVER), true);
			QThread* sslWsThread = new QThread(this);
			sslWsThread->setObjectName("SSLWebServerThread");
			_sslWebserver->moveToThread(sslWsThread);
			connect(sslWsThread, &QThread::started, _sslWebserver, &WebServer::initServer);
			connect(sslWsThread, &QThread::finished, _sslWebserver, &WebServer::deleteLater);
			connect(this, &HyperionDaemon::settingsChanged, _sslWebserver, &WebServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
			connect(_sslWebserver, &WebServer::publishService, _mDNSProvider, &MdnsProvider::publishService);
#endif
			sslWsThread->start();
		}
		break;

	case InstanceState::H_STOPPED:
		break;

	default:
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

void HyperionDaemon::freeObjects()
{
	Debug(_log, "Cleaning up Hyperion before quit.");

#ifdef ENABLE_MDNS
	if (_mDNSProvider != nullptr)
	{
		auto mDnsThread = _mDNSProvider->thread();
		mDnsThread->quit();
		mDnsThread->wait();
		delete mDnsThread;
		_mDNSProvider = nullptr;
	}
#endif

	if (_jsonServer != nullptr)
	{
		auto jsonThread = _jsonServer->thread();
		jsonThread->quit();
		jsonThread->wait();
		delete jsonThread;
		_jsonServer = nullptr;
	}

#if defined(ENABLE_FLATBUF_SERVER)
	if (_flatBufferServer != nullptr)
	{
		auto flatBufferServerThread = _flatBufferServer->thread();
		flatBufferServerThread->quit();
		flatBufferServerThread->wait();
		delete flatBufferServerThread;
		_flatBufferServer = nullptr;
	}
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	if (_protoServer != nullptr)
	{
		auto protoServerThread = _protoServer->thread();
		protoServerThread->quit();
		protoServerThread->wait();
		delete protoServerThread;
		_protoServer = nullptr;
	}
#endif

	//ssdp before webserver
	if (_ssdp != nullptr)
	{
		auto ssdpThread = _ssdp->thread();
		ssdpThread->quit();
		ssdpThread->wait();
		delete ssdpThread;
		_ssdp = nullptr;
	}

	if (_webserver != nullptr)
	{
		auto webserverThread = _webserver->thread();
		webserverThread->quit();
		webserverThread->wait();
		delete webserverThread;
		_webserver = nullptr;
	}

	if (_sslWebserver != nullptr)
	{
		auto sslWebserverThread = _sslWebserver->thread();
		sslWebserverThread->quit();
		sslWebserverThread->wait();
		delete sslWebserverThread;
		_sslWebserver = nullptr;
	}

#ifdef ENABLE_CEC
	if (_cecHandler != nullptr)
	{
		auto cecHandlerThread = _cecHandler->thread();
		cecHandlerThread->quit();
		cecHandlerThread->wait();
		delete cecHandlerThread;
		delete _cecHandler;
		_cecHandler = nullptr;
	}
#endif

	delete _suspendHandler;

	// stop Hyperions (non blocking)
	_instanceManager->stopAll();

	delete _amlGrabber;
	if (_dispmanx != nullptr)
		delete _dispmanx;
	delete _fbGrabber;
	delete _osxGrabber;
	delete _qtGrabber;
	delete _dxGrabber;
	delete _videoGrabber;
	delete _audioGrabber;

	_videoGrabber = nullptr;
	_amlGrabber = nullptr;
	_dispmanx = nullptr;
	_fbGrabber = nullptr;
	_osxGrabber = nullptr;
	_qtGrabber = nullptr;
	_dxGrabber = nullptr;
	_audioGrabber = nullptr;
}

void HyperionDaemon::startNetworkServices()
{
	// Create mDNS-Provider in thread to allow publishing other services
#ifdef ENABLE_MDNS
	_mDNSProvider = new MdnsProvider();
	QThread* mDnsThread = new QThread(this);
	mDnsThread->setObjectName("mDNSProviderThread");
	_mDNSProvider->moveToThread(mDnsThread);
	connect(mDnsThread, &QThread::started, _mDNSProvider, &MdnsProvider::init);
	connect(mDnsThread, &QThread::finished, _mDNSProvider, &MdnsProvider::deleteLater);
	mDnsThread->start();
#endif

#if defined(ENABLE_FLATBUF_SERVER)
	// Create FlatBuffer server in thread
	_flatBufferServer = new FlatBufferServer(getSetting(settings::FLATBUFSERVER));
	QThread* fbThread = new QThread(this);
	fbThread->setObjectName("FlatBufferServerThread");
	_flatBufferServer->moveToThread(fbThread);
	connect(fbThread, &QThread::started, _flatBufferServer, &FlatBufferServer::initServer);
	connect(fbThread, &QThread::finished, _flatBufferServer, &FlatBufferServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _flatBufferServer, &FlatBufferServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
	connect(_flatBufferServer, &FlatBufferServer::publishService, _mDNSProvider, &MdnsProvider::publishService);
#endif
	fbThread->start();
#endif

#if defined(ENABLE_PROTOBUF_SERVER)
	// Create Proto server in thread
	_protoServer = new ProtoServer(getSetting(settings::PROTOSERVER));
	QThread* pThread = new QThread(this);
	pThread->setObjectName("ProtoServerThread");
	_protoServer->moveToThread(pThread);
	connect(pThread, &QThread::started, _protoServer, &ProtoServer::initServer);
	connect(pThread, &QThread::finished, _protoServer, &ProtoServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _protoServer, &ProtoServer::handleSettingsUpdate);
#ifdef ENABLE_MDNS
	connect(_protoServer, &ProtoServer::publishService, _mDNSProvider, &MdnsProvider::publishService);
#endif
	pThread->start();
#endif

	// Create Webserver
	_webserver = new WebServer(getSetting(settings::WEBSERVER), false);

	// Create SSDP server
	_ssdp = new SSDPHandler(_webserver,
		getSetting(settings::FLATBUFSERVER).object()["port"].toInt(),
		getSetting(settings::PROTOSERVER).object()["port"].toInt(),
		getSetting(settings::JSONSERVER).object()["port"].toInt(),
		getSetting(settings::WEBSERVER).object()["sslPort"].toInt(),
		getSetting(settings::GENERAL).object()["name"].toString());
	QThread* ssdpThread = new QThread(this);
	ssdpThread->setObjectName("SSDPThread");
	_ssdp->moveToThread(ssdpThread);
	connect(ssdpThread, &QThread::started, _ssdp, &SSDPHandler::initServer);
	connect(ssdpThread, &QThread::finished, _ssdp, &SSDPHandler::deleteLater);
	connect(_webserver, &WebServer::stateChange, _ssdp, &SSDPHandler::handleWebServerStateChange);
	connect(this, &HyperionDaemon::settingsChanged, _ssdp, &SSDPHandler::handleSettingsUpdate);
	ssdpThread->start();
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
		const QJsonObject& grabberConfig = config.object();

		_grabber_width = grabberConfig["width"].toInt(96);
		_grabber_height = grabberConfig["height"].toInt(96);
		_grabber_pixelDecimation = grabberConfig["pixelDecimation"].toInt(GrabberWrapper::DEFAULT_PIXELDECIMATION);
		_grabber_frequency = grabberConfig["fps"].toInt(GrabberWrapper::DEFAULT_RATE_HZ);

		_grabber_cropLeft = grabberConfig["cropLeft"].toInt(0);
		_grabber_cropRight = grabberConfig["cropRight"].toInt(0);
		_grabber_cropTop = grabberConfig["cropTop"].toInt(0);
		_grabber_cropBottom = grabberConfig["cropBottom"].toInt(0);

#ifdef ENABLE_OSX
		QString type = grabberConfig["device"].toString("osx");
#else
		QString type = grabberConfig["device"].toString("auto");
#endif

		// auto eval of type
		if (type == "auto")
		{
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
		}

		if (_prevType != type)
		{
			// stop all capture interfaces
#ifdef ENABLE_FB
			if (_fbGrabber != nullptr)
			{
				_fbGrabber->stop();
				delete _fbGrabber;
				_fbGrabber = nullptr;
			}
#endif
#ifdef ENABLE_DISPMANX
			if (_dispmanx != nullptr)
			{
				_dispmanx->stop();
				delete _dispmanx;
				_dispmanx = nullptr;
			}
#endif
#ifdef ENABLE_AMLOGIC
			if (_amlGrabber != nullptr)
			{
				_amlGrabber->stop();
				delete _amlGrabber;
				_amlGrabber = nullptr;
			}
#endif
#ifdef ENABLE_OSX
			if (_osxGrabber != nullptr)
			{
				_osxGrabber->stop();
				delete _osxGrabber;
				_osxGrabber = nullptr;
			}
#endif
#ifdef ENABLE_X11
			if (_x11Grabber != nullptr)
			{
				_x11Grabber->stop();
				delete _x11Grabber;
				_x11Grabber = nullptr;
			}
#endif
#ifdef ENABLE_XCB
			if (_xcbGrabber != nullptr)
			{
				_xcbGrabber->stop();
				delete _xcbGrabber;
				_xcbGrabber = nullptr;
			}
#endif
#ifdef ENABLE_QT
			if (_qtGrabber != nullptr)
			{
				_qtGrabber->stop();
				delete _qtGrabber;
				_qtGrabber = nullptr;
			}
#endif
#ifdef ENABLE_DX
			if (_dxGrabber != nullptr)
			{
				_dxGrabber->stop();
				delete _dxGrabber;
				_dxGrabber = nullptr;
			}
#endif

			// create/start capture interface
			if (type == "framebuffer")
			{
				if (_fbGrabber == nullptr)
				{
					createGrabberFramebuffer(grabberConfig);
				}
#ifdef ENABLE_FB
				_fbGrabber->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
				_fbGrabber->tryStart();
#endif
			}
			else if (type == "dispmanx")
			{
				if (_dispmanx == nullptr)
				{
					createGrabberDispmanx(grabberConfig);
				}

#ifdef ENABLE_DISPMANX
				if (_dispmanx != nullptr)
				{
					_dispmanx->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
					_dispmanx->tryStart();
				}
#endif
			}
			else if (type == "amlogic")
			{
				if (_amlGrabber == nullptr)
				{
					createGrabberAmlogic(grabberConfig);
				}
#ifdef ENABLE_AMLOGIC
				_amlGrabber->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
				_amlGrabber->tryStart();
#endif
			}
			else if (type == "osx")
			{
				if (_osxGrabber == nullptr)
				{
					createGrabberOsx(grabberConfig);
				}
#ifdef ENABLE_OSX
				_osxGrabber->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
				_osxGrabber->tryStart();
#endif
			}
			else if (type == "x11")
			{
				if (_x11Grabber == nullptr)
				{
					createGrabberX11(grabberConfig);
				}
#ifdef ENABLE_X11
				_x11Grabber->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
				_x11Grabber->tryStart();
#endif
			}
			else if (type == "xcb")
			{
				if (_xcbGrabber == nullptr)
				{
					createGrabberXcb(grabberConfig);
				}
#ifdef ENABLE_XCB
				_xcbGrabber->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
				_xcbGrabber->tryStart();
#endif
			}
			else if (type == "qt")
			{
				if (_qtGrabber == nullptr)
				{
					createGrabberQt(grabberConfig);
				}
#ifdef ENABLE_QT
				_qtGrabber->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
				_qtGrabber->tryStart();
#endif
			}
			else if (type == "dx")
			{
				if (_dxGrabber == nullptr)
				{
					createGrabberDx(grabberConfig);
				}
#ifdef ENABLE_DX
				_dxGrabber->handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));
				_dxGrabber->tryStart();
#endif
			}
			else
			{
				Error(_log, "Unknown platform capture type: %s", QSTRING_CSTR(type));
				return;
			}
			_prevType = type;
		}
	}
	else if (settingsType == settings::V4L2)
	{

#ifdef ENABLE_CEC
		const QJsonObject& grabberConfig = config.object();
		if (_cecHandler != nullptr && grabberConfig["cecDetection"].toBool(false))
		{
			QMetaObject::invokeMethod(_cecHandler, "start", Qt::QueuedConnection);
		}
		else
		{
			QMetaObject::invokeMethod(_cecHandler, "stop", Qt::QueuedConnection);
		}
#endif

#if defined(ENABLE_V4L2) || defined(ENABLE_MF)
		if (_videoGrabber == nullptr)
		{
			_videoGrabber = new VideoWrapper();
			_videoGrabber->handleSettingsUpdate(settings::V4L2, getSetting(settings::V4L2));

#if defined(ENABLE_MF)
			Debug(_log, "Media Foundation grabber created");
#elif defined(ENABLE_V4L2)
			Debug(_log, "V4L2 grabber created");
#endif

			// connect to HyperionDaemon signal
			connect(this, &HyperionDaemon::videoMode, _videoGrabber, &VideoWrapper::setVideoMode);
			connect(this, &HyperionDaemon::settingsChanged, _videoGrabber, &VideoWrapper::handleSettingsUpdate);
		}
#else
		Debug(_log, "The v4l2 grabber is not supported on this platform");
#endif
	}
	else if (settingsType == settings::AUDIO)
	{
#ifdef ENABLE_AUDIO
		// Create Audio Grabber
		if (_audioGrabber == nullptr)
		{
			_audioGrabber = new AudioWrapper();
			_audioGrabber->handleSettingsUpdate(settings::AUDIO, getSetting(settings::AUDIO));

			connect(this, &HyperionDaemon::settingsChanged, _audioGrabber, &AudioWrapper::handleSettingsUpdate);

			Debug(_log, "Audio grabber created");
		}
#else
		Debug(_log, "Audio capture not supported on this platform");
#endif
	}
}

void HyperionDaemon::createGrabberDispmanx(const QJsonObject& /*grabberConfig*/)
{
#ifdef ENABLE_DISPMANX
	_dispmanx = new DispmanxWrapper(
		_grabber_frequency,
		_grabber_pixelDecimation
	);

	if (!_dispmanx->available)
	{
		delete _dispmanx;
		_dispmanx = nullptr;
		Debug(_log, "The dispmanx framegrabber is not supported on this platform");
		return;
	}

	_dispmanx->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _dispmanx, &DispmanxWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _dispmanx, &DispmanxWrapper::handleSettingsUpdate);

	Info(_log, "DISPMANX frame grabber created");
#else
	Debug(_log, "The dispmanx framegrabber is not supported on this platform");
#endif
}

void HyperionDaemon::createGrabberAmlogic(const QJsonObject& /*grabberConfig*/)
{
#ifdef ENABLE_AMLOGIC
	_amlGrabber = new AmlogicWrapper(
		_grabber_frequency,
		_grabber_pixelDecimation
	);
	_amlGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _amlGrabber, &AmlogicWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _amlGrabber, &AmlogicWrapper::handleSettingsUpdate);

	Info(_log, "AMLOGIC grabber created");
#else
	Debug(_log, "The AMLOGIC grabber is not supported on this platform");
#endif
}

void HyperionDaemon::createGrabberX11(const QJsonObject& /*grabberConfig*/)
{
#ifdef ENABLE_X11
	_x11Grabber = new X11Wrapper(
		_grabber_frequency,
		_grabber_pixelDecimation,
		_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom
	);
	_x11Grabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _x11Grabber, &X11Wrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _x11Grabber, &X11Wrapper::handleSettingsUpdate);

	Info(_log, "X11 grabber created");
#else
	Debug(_log, "The X11 grabber is not supported on this platform");
#endif
}

void HyperionDaemon::createGrabberXcb(const QJsonObject& /*grabberConfig*/)
{
#ifdef ENABLE_XCB
	_xcbGrabber = new XcbWrapper(
		_grabber_frequency,
		_grabber_pixelDecimation,
		_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom
	);
	_xcbGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _xcbGrabber, &XcbWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _xcbGrabber, &XcbWrapper::handleSettingsUpdate);

	Info(_log, "XCB grabber created");
#else
	Debug(_log, "The XCB grabber is not supported on this platform");
#endif
}

void HyperionDaemon::createGrabberQt(const QJsonObject& grabberConfig)
{
#ifdef ENABLE_QT
	_qtGrabber = new QtWrapper(
		_grabber_frequency,
		grabberConfig["input"].toInt(0),
		_grabber_pixelDecimation,
		_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom
	);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _qtGrabber, &QtWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _qtGrabber, &QtWrapper::handleSettingsUpdate);

	Info(_log, "Qt grabber created");
#else
	Debug(_log, "The Qt grabber is not supported on this platform");
#endif
}

void HyperionDaemon::createGrabberDx(const QJsonObject& grabberConfig)
{
#ifdef ENABLE_DX
	_dxGrabber = new DirectXWrapper(
		_grabber_frequency,
		grabberConfig["display"].toInt(0),
		_grabber_pixelDecimation,
		_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom
	);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _dxGrabber, &DirectXWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _dxGrabber, &DirectXWrapper::handleSettingsUpdate);

	Info(_log, "DirectX grabber created");
#else
	Debug(_log, "The DirectX is not supported on this platform");
#endif
}

void HyperionDaemon::createGrabberFramebuffer(const QJsonObject& grabberConfig)
{
#ifdef ENABLE_FB
	// Construct and start the framebuffer grabber if the configuration is present

	int fbIdx = grabberConfig["input"].toInt(0);
	QString devicePath = QString("/dev/fb%1").arg(fbIdx);
	_fbGrabber = new FramebufferWrapper(
		_grabber_frequency,
		devicePath,
		_grabber_pixelDecimation
	);
	_fbGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);
	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _fbGrabber, &FramebufferWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _fbGrabber, &FramebufferWrapper::handleSettingsUpdate);

	Info(_log, "Framebuffer grabber created");
#else
	Debug(_log, "The framebuffer is not supported on this platform");
#endif
}

void HyperionDaemon::createGrabberOsx(const QJsonObject& grabberConfig)
{
#ifdef ENABLE_OSX
	// Construct and start the osx grabber if the configuration is present
	_osxGrabber = new OsxWrapper(
		_grabber_frequency,
		grabberConfig["input"].toInt(0),
		_grabber_pixelDecimation
	);
	_osxGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _osxGrabber, &OsxWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _osxGrabber, &OsxWrapper::handleSettingsUpdate);

	Info(_log, "OSX grabber created");
#else
	Debug(_log, "The osx grabber is not supported on this platform");
#endif
}

void HyperionDaemon::createCecHandler()
{
#if defined(ENABLE_V4L2) && defined(ENABLE_CEC)
	_cecHandler = new CECHandler;

	QThread* thread = new QThread(this);
	thread->setObjectName("CECThread");
	_cecHandler->moveToThread(thread);
	thread->start();

	connect(_cecHandler, &CECHandler::cecEvent, [&](CECEvent event) {
		if (_videoGrabber != nullptr)
		{
			_videoGrabber->handleCecEvent(event);
		}
		});

	Info(_log, "CEC handler created");
#else
	Debug(_log, "The CEC handler is not supported on this platform");
#endif
}

