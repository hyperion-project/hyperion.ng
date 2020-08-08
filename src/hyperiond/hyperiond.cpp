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

#include <HyperionConfig.h> // Required to determine the cmake options

// bonjour browser
#ifdef ENABLE_AVAHI
#include <bonjour/bonjourbrowserwrapper.h>
#endif
#include <jsonserver/JsonServer.h>
#include <webserver/WebServer.h>
#include "hyperiond.h"

// Flatbuffer Server
#include <flatbufserver/FlatBufferServer.h>

// Protobuffer Server
#include <protoserver/ProtoServer.h>

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

// Init Python
#include <python/PythonInit.h>

// EffectFileHandler
#include <effectengine/EffectFileHandler.h>

#ifdef ENABLE_CEC
#include <cec/CECHandler.h>
#endif

HyperionDaemon *HyperionDaemon::daemon = nullptr;

HyperionDaemon::HyperionDaemon(const QString rootPath, QObject *parent, bool logLvlOverwrite)
		: QObject(parent), _log(Logger::getInstance("DAEMON"))
		, _instanceManager(new HyperionIManager(rootPath, this))
		, _authManager(new AuthManager(this))
#ifdef ENABLE_AVAHI
		, _bonjourBrowserWrapper(new BonjourBrowserWrapper())
#endif
		, _netOrigin(new NetOrigin(this))
		, _pyInit(new PythonInit())
		, _webserver(nullptr)
		, _sslWebserver(nullptr)
		, _jsonServer(nullptr)
		, _v4l2Grabber(nullptr)
		, _dispmanx(nullptr)
		, _x11Grabber(nullptr)
		, _xcbGrabber(nullptr)
		, _amlGrabber(nullptr)
		, _fbGrabber(nullptr)
		, _osxGrabber(nullptr)
		, _qtGrabber(nullptr)
		, _ssdp(nullptr)
		, _cecHandler(nullptr)
		, _currVideoMode(VideoMode::VIDEO_2D)
		, _rootPath(rootPath)
{
	HyperionDaemon::daemon = this;

	// Register metas for thread queued connection
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");
	qRegisterMetaType<settings::type>("settings::type");
	qRegisterMetaType<VideoMode>("VideoMode");
	qRegisterMetaType<QMap<quint8, QJsonObject>>("QMap<quint8,QJsonObject>");
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");

	// init settings
	_settingsManager = new SettingsManager(0, this);

	// set inital log lvl if the loglvl wasn't overwritten by arg
	if (!logLvlOverwrite)
		handleSettingsUpdate(settings::LOGGER, getSetting(settings::LOGGER));

	createCecHandler();

	// init EffectFileHandler
	EffectFileHandler *efh = new EffectFileHandler(rootPath, getSetting(settings::EFFECTS), this);
	connect(this, &HyperionDaemon::settingsChanged, efh, &EffectFileHandler::handleSettingsUpdate);

	// connect and apply settings for AuthManager
	connect(this, &HyperionDaemon::settingsChanged, _authManager, &AuthManager::handleSettingsUpdate);
	_authManager->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

	// connect and apply settings for NetOrigin
	connect(this, &HyperionDaemon::settingsChanged, _netOrigin, &NetOrigin::handleSettingsUpdate);
	_netOrigin->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

	// spawn all Hyperion instances (non blocking)
	_instanceManager->startAll();

	//Cleaning up Hyperion before quit
	connect(parent, SIGNAL(aboutToQuit()), this, SLOT(freeObjects()));

	// pipe settings changes and component state changes from HyperionIManager to Daemon
	connect(_instanceManager, &HyperionIManager::settingsChanged, this, &HyperionDaemon::settingsChanged);
	connect(_instanceManager, &HyperionIManager::compStateChangeRequest, this, &HyperionDaemon::compStateChangeRequest);

	// listen for setting changes of framegrabber and v4l2
	connect(this, &HyperionDaemon::settingsChanged, this, &HyperionDaemon::handleSettingsUpdate);

	// forward videoModes from HyperionIManager to Daemon evaluation
	connect(_instanceManager, &HyperionIManager::requestVideoMode, this, &HyperionDaemon::setVideoMode);
	// return videoMode changes from Daemon to HyperionIManager
	connect(this, &HyperionDaemon::videoMode, _instanceManager, &HyperionIManager::newVideoMode);

// ---- grabber -----
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB) && !defined(ENABLE_X11) && !defined(ENABLE_XCB) && !defined(ENABLE_AMLOGIC) && !defined(ENABLE_QT)
	Warning(_log, "No platform capture can be instantiated, because all grabbers have been left out from the build");
#endif

	// init system capture (framegrabber)
	handleSettingsUpdate(settings::SYSTEMCAPTURE, getSetting(settings::SYSTEMCAPTURE));

	// init v4l2 capture
	handleSettingsUpdate(settings::V4L2, getSetting(settings::V4L2));

	// ---- network services -----
	startNetworkServices();		
}

HyperionDaemon::~HyperionDaemon()
{
	delete _settingsManager;
	delete _pyInit;
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

	// destroy network first as a client might want to access hyperion
	delete _jsonServer;
	_jsonServer = nullptr;

	if (_flatBufferServer)
	{
		auto flatBufferServerThread = _flatBufferServer->thread();
		flatBufferServerThread->quit();
		flatBufferServerThread->wait();
		delete flatBufferServerThread;
		_flatBufferServer = nullptr;
	}

	if (_protoServer)
	{
		auto protoServerThread = _protoServer->thread();
		protoServerThread->quit();
		protoServerThread->wait();
		delete protoServerThread;
		_protoServer = nullptr;
	}

	//ssdp before webserver
	if (_ssdp)
	{
		auto ssdpThread = _ssdp->thread();
		ssdpThread->quit();
		ssdpThread->wait();
		delete ssdpThread;
		_ssdp = nullptr;
	}

	if(_webserver)
	{
		auto webserverThread =_webserver->thread();
		webserverThread->quit();
		webserverThread->wait();
		delete webserverThread;
		_webserver = nullptr;
	}

	if (_sslWebserver)
	{
		auto sslWebserverThread =_sslWebserver->thread();
		sslWebserverThread->quit();
		sslWebserverThread->wait();
		delete sslWebserverThread;
		_sslWebserver = nullptr;
	}

#ifdef ENABLE_CEC
	if (_cecHandler)
	{
		auto cecHandlerThread = _cecHandler->thread();
		cecHandlerThread->quit();
		cecHandlerThread->wait();
		delete cecHandlerThread;
		delete _cecHandler;
		_cecHandler = nullptr;
	}
#endif

	// stop Hyperions (non blocking)
	_instanceManager->stopAll();

	delete _bonjourBrowserWrapper;
	delete _amlGrabber;
	delete _dispmanx;
	delete _fbGrabber;
	delete _osxGrabber;
	delete _qtGrabber;
	delete _v4l2Grabber;

	_v4l2Grabber = nullptr;
	_bonjourBrowserWrapper = nullptr;
	_amlGrabber = nullptr;
	_dispmanx = nullptr;
	_fbGrabber = nullptr;
	_osxGrabber = nullptr;
	_qtGrabber = nullptr;
}

void HyperionDaemon::startNetworkServices()
{
	// Create Json server
	_jsonServer = new JsonServer(getSetting(settings::JSONSERVER));
	connect(this, &HyperionDaemon::settingsChanged, _jsonServer, &JsonServer::handleSettingsUpdate);

	// Create FlatBuffer server in thread
	_flatBufferServer = new FlatBufferServer(getSetting(settings::FLATBUFSERVER));
	QThread *fbThread = new QThread(this);
	fbThread->setObjectName("FlatBufferServerThread");
	_flatBufferServer->moveToThread(fbThread);
	connect(fbThread, &QThread::started, _flatBufferServer, &FlatBufferServer::initServer);
	connect(fbThread, &QThread::finished, _flatBufferServer, &FlatBufferServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _flatBufferServer, &FlatBufferServer::handleSettingsUpdate);
	fbThread->start();

	// Create Proto server in thread
	_protoServer = new ProtoServer(getSetting(settings::PROTOSERVER));
	QThread *pThread = new QThread(this);
	pThread->setObjectName("ProtoServerThread");
	_protoServer->moveToThread(pThread);
	connect(pThread, &QThread::started, _protoServer, &ProtoServer::initServer);
	connect(pThread, &QThread::finished, _protoServer, &ProtoServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _protoServer, &ProtoServer::handleSettingsUpdate);
	pThread->start();

	// Create Webserver in thread
	_webserver = new WebServer(getSetting(settings::WEBSERVER), false);
	QThread *wsThread = new QThread(this);
	wsThread->setObjectName("WebServerThread");
	_webserver->moveToThread(wsThread);
	connect(wsThread, &QThread::started, _webserver, &WebServer::initServer);
	connect(wsThread, &QThread::finished, _webserver, &WebServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _webserver, &WebServer::handleSettingsUpdate);
	wsThread->start();

	// Create SSL Webserver in thread
	_sslWebserver = new WebServer(getSetting(settings::WEBSERVER), true);
	QThread *sslWsThread = new QThread(this);
	sslWsThread->setObjectName("SSLWebServerThread");
	_sslWebserver->moveToThread(sslWsThread);
	connect(sslWsThread, &QThread::started, _sslWebserver, &WebServer::initServer);
	connect(sslWsThread, &QThread::finished, _sslWebserver, &WebServer::deleteLater);
	connect(this, &HyperionDaemon::settingsChanged, _sslWebserver, &WebServer::handleSettingsUpdate);
	sslWsThread->start();

	// Create SSDP server in thread
	_ssdp = new SSDPHandler(_webserver, getSetting(settings::FLATBUFSERVER).object()["port"].toInt(), getSetting(settings::JSONSERVER).object()["port"].toInt(), getSetting(settings::GENERAL).object()["name"].toString());
	QThread *ssdpThread = new QThread(this);
	ssdpThread->setObjectName("SSDPThread");
	_ssdp->moveToThread(ssdpThread);
	connect(ssdpThread, &QThread::started, _ssdp, &SSDPHandler::initServer);
	connect(ssdpThread, &QThread::finished, _ssdp, &SSDPHandler::deleteLater);
	connect(_webserver, &WebServer::stateChange, _ssdp, &SSDPHandler::handleWebServerStateChange);
	connect(this, &HyperionDaemon::settingsChanged, _ssdp, &SSDPHandler::handleSettingsUpdate);
	ssdpThread->start();
}

void HyperionDaemon::handleSettingsUpdate(settings::type settingsType, const QJsonDocument &config)
{
	if (settingsType == settings::LOGGER)
	{
		const QJsonObject &logConfig = config.object();

		std::string level = logConfig["level"].toString("warn").toStdString(); // silent warn verbose debug
		if (level == "silent")
			Logger::setLogLevel(Logger::OFF);
		else if (level == "warn")
			Logger::setLogLevel(Logger::LogLevel::WARNING);
		else if (level == "verbose")
			Logger::setLogLevel(Logger::INFO);
		else if (level == "debug")
			Logger::setLogLevel(Logger::DEBUG);
	}

	if (settingsType == settings::SYSTEMCAPTURE)
	{
		const QJsonObject &grabberConfig = config.object();

		_grabber_width = grabberConfig["width"].toInt(96);
		_grabber_height = grabberConfig["height"].toInt(96);
		_grabber_frequency = grabberConfig["frequency_Hz"].toInt(10);

		_grabber_cropLeft = grabberConfig["cropLeft"].toInt(0);
		_grabber_cropRight = grabberConfig["cropRight"].toInt(0);
		_grabber_cropTop = grabberConfig["cropTop"].toInt(0);
		_grabber_cropBottom = grabberConfig["cropBottom"].toInt(0);

		_grabber_ge2d_mode = grabberConfig["ge2d_mode"].toInt(0);
		_grabber_device = grabberConfig["amlogic_grabber"].toString("amvideocap0");

#ifdef ENABLE_OSX
		QString type = "osx";
#else
		QString type = grabberConfig["type"].toString("auto");
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
			else if (QFile::exists("/dev/amvideo"))
			{
				type = "amlogic";

				if (!QFile::exists("/dev/" + _grabber_device))
				{
					Error(_log, "grabber device '%s' for type amlogic not found!", QSTRING_CSTR(_grabber_device));
				}
			}
			else
			{
				// x11 -> if DISPLAY is set
				QByteArray envDisplay = qgetenv("DISPLAY");
				if ( !envDisplay.isEmpty() )
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

		if (_prevType != type)
		{
			Info(_log, "set screen capture device to '%s'", QSTRING_CSTR(type));

			// stop all capture interfaces
			#ifdef ENABLE_FB
			if(_fbGrabber != nullptr)
			{
				_fbGrabber->stop();
				delete _fbGrabber;
				_fbGrabber = nullptr;
			}
			#endif
			#ifdef ENABLE_DISPMANX
			if(_dispmanx != nullptr)
			{
				_dispmanx->stop();
				delete _dispmanx;
				_dispmanx = nullptr;
			}
			#endif
			#ifdef ENABLE_AMLOGIC
			if(_amlGrabber != nullptr)
			{
				_amlGrabber->stop();
				delete _amlGrabber;
				_amlGrabber = nullptr;
			}
			#endif
			#ifdef ENABLE_OSX
			if(_osxGrabber != nullptr)
			{
				_osxGrabber->stop();
				delete _osxGrabber;
				_osxGrabber = nullptr;
			}
			#endif
			#ifdef ENABLE_X11
			if(_x11Grabber != nullptr)
			{
				 _x11Grabber->stop();
				 delete _x11Grabber;
				 _x11Grabber = nullptr;
			}
			#endif
			#ifdef ENABLE_XCB
			if(_xcbGrabber != nullptr)
			{
				 _xcbGrabber->stop();
				 delete _xcbGrabber;
				 _xcbGrabber = nullptr;
			}
			#endif
			#ifdef ENABLE_QT
			if(_qtGrabber != nullptr)
			{
				_qtGrabber->stop();
				delete _qtGrabber;
				_qtGrabber = nullptr;
			}
			#endif

			// create/start capture interface
			if (type == "framebuffer")
			{
				if (_fbGrabber == nullptr)
					createGrabberFramebuffer(grabberConfig);
				#ifdef ENABLE_FB
				_fbGrabber->tryStart();
				#endif
			}
			else if (type == "dispmanx")
			{
				if (_dispmanx == nullptr)
					createGrabberDispmanx();
				#ifdef ENABLE_DISPMANX
				_dispmanx->tryStart();
				#endif
			}
			else if (type == "amlogic")
			{
				if (_amlGrabber == nullptr)
					createGrabberAmlogic();
				#ifdef ENABLE_AMLOGIC
				_amlGrabber->tryStart();
				#endif
			}
			else if (type == "osx")
			{
				if (_osxGrabber == nullptr)
					createGrabberOsx(grabberConfig);
				#ifdef ENABLE_OSX
				_osxGrabber->tryStart();
				#endif
			}
			else if (type == "x11")
			{
				if (_x11Grabber == nullptr)
					createGrabberX11(grabberConfig);
				#ifdef ENABLE_X11
				_x11Grabber->tryStart();
				#endif
			}
			else if (type == "xcb")
			{
				if (_xcbGrabber == nullptr)
					createGrabberXcb(grabberConfig);
				#ifdef ENABLE_XCB
				_xcbGrabber->tryStart();
				#endif
			}
			else if (type == "qt")
			{
				if (_qtGrabber == nullptr)
					createGrabberQt(grabberConfig);
				#ifdef ENABLE_QT
				_qtGrabber->tryStart();
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
		const QJsonObject &grabberConfig = config.object();
#ifdef ENABLE_CEC
		QString operation;
		if (_cecHandler && grabberConfig["cecDetection"].toBool(false))
		{
			QMetaObject::invokeMethod(_cecHandler, "start", Qt::QueuedConnection);
		}
		else
		{
			QMetaObject::invokeMethod(_cecHandler, "stop", Qt::QueuedConnection);
		}
#endif

		if (_v4l2Grabber != nullptr)
			return;

#ifdef ENABLE_V4L2
		_v4l2Grabber = new V4L2Wrapper(
				grabberConfig["device"].toString("auto"),
				grabberConfig["width"].toInt(0),
				grabberConfig["height"].toInt(0),
				grabberConfig["fps"].toInt(15),
				grabberConfig["input"].toInt(-1),
				parseVideoStandard(grabberConfig["standard"].toString("no-change")),
				parsePixelFormat(grabberConfig["pixelFormat"].toString("no-change")),
				grabberConfig["sizeDecimation"].toInt(8));
				
		// HDR stuff		
		_v4l2Grabber->loadLutFile(_rootPath);				
		_v4l2Grabber->setHdrToneMappingEnabled(grabberConfig["hdrToneMapping"].toBool(false));
		
		// software frame skipping
		_v4l2Grabber->setFpsSoftwareDecimation(grabberConfig["fpsSoftwareDecimation"].toInt(1));
		
		_v4l2Grabber->setSignalThreshold(
				grabberConfig["redSignalThreshold"].toDouble(0.0) / 100.0,
				grabberConfig["greenSignalThreshold"].toDouble(0.0) / 100.0,
				grabberConfig["blueSignalThreshold"].toDouble(0.0) / 100.0);
		_v4l2Grabber->setCropping(
				grabberConfig["cropLeft"].toInt(0),
				grabberConfig["cropRight"].toInt(0),
				grabberConfig["cropTop"].toInt(0),
				grabberConfig["cropBottom"].toInt(0));

		_v4l2Grabber->setCecDetectionEnable(grabberConfig["cecDetection"].toBool(true));
		_v4l2Grabber->setSignalDetectionEnable(grabberConfig["signalDetection"].toBool(true));
		_v4l2Grabber->setSignalDetectionOffset(
				grabberConfig["sDHOffsetMin"].toDouble(0.25),
				grabberConfig["sDVOffsetMin"].toDouble(0.25),
				grabberConfig["sDHOffsetMax"].toDouble(0.75),
				grabberConfig["sDVOffsetMax"].toDouble(0.75));
		Debug(_log, "V4L2 grabber created");

		// connect to HyperionDaemon signal
		connect(this, &HyperionDaemon::videoMode, _v4l2Grabber, &V4L2Wrapper::setVideoMode);
		connect(this, &HyperionDaemon::settingsChanged, _v4l2Grabber, &V4L2Wrapper::handleSettingsUpdate);
#else
		Error(_log, "The v4l2 grabber can not be instantiated, because it has been left out from the build");
#endif
	}
}

void HyperionDaemon::createGrabberDispmanx()
{
#ifdef ENABLE_DISPMANX
	_dispmanx = new DispmanxWrapper(_grabber_width, _grabber_height, _grabber_frequency);
	_dispmanx->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _dispmanx, &DispmanxWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _dispmanx, &DispmanxWrapper::handleSettingsUpdate);

	Info(_log, "DISPMANX frame grabber created");
#else
	Error(_log, "The dispmanx framegrabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberAmlogic()
{
#ifdef ENABLE_AMLOGIC
	_amlGrabber = new AmlogicWrapper(_grabber_width, _grabber_height);
	_amlGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _amlGrabber, &AmlogicWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _amlGrabber, &AmlogicWrapper::handleSettingsUpdate);

	Info(_log, "AMLOGIC grabber created");
#else
	Error(_log, "The AMLOGIC grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberX11(const QJsonObject &grabberConfig)
{
#ifdef ENABLE_X11
	_x11Grabber = new X11Wrapper(
			_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom,
			grabberConfig["pixelDecimation"].toInt(8),
			_grabber_frequency);
	_x11Grabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _x11Grabber, &X11Wrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _x11Grabber, &X11Wrapper::handleSettingsUpdate);

	Info(_log, "X11 grabber created");
#else
	Error(_log, "The X11 grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberXcb(const QJsonObject &grabberConfig)
{
#ifdef ENABLE_XCB
	_xcbGrabber = new XcbWrapper(
			_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom,
			grabberConfig["pixelDecimation"].toInt(8),
			_grabber_frequency);
	_xcbGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _xcbGrabber, &XcbWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _xcbGrabber, &XcbWrapper::handleSettingsUpdate);

	Info(_log, "XCB grabber created");
#else
	Error(_log, "The XCB grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberQt(const QJsonObject &grabberConfig)
{
#ifdef ENABLE_QT
	_qtGrabber = new QtWrapper(
			_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom,
			grabberConfig["pixelDecimation"].toInt(8),
			grabberConfig["display"].toInt(0),
			_grabber_frequency);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _qtGrabber, &QtWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _qtGrabber, &QtWrapper::handleSettingsUpdate);

	Info(_log, "Qt grabber created");
#else
	Error(_log, "The Qt grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberFramebuffer(const QJsonObject &grabberConfig)
{
#ifdef ENABLE_FB
	// Construct and start the framebuffer grabber if the configuration is present
	_fbGrabber = new FramebufferWrapper(
			grabberConfig["device"].toString("/dev/fb0"),
			_grabber_width, _grabber_height, _grabber_frequency);
	_fbGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);
	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _fbGrabber, &FramebufferWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _fbGrabber, &FramebufferWrapper::handleSettingsUpdate);

	Info(_log, "Framebuffer grabber created");
#else
	Error(_log, "The framebuffer grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberOsx(const QJsonObject &grabberConfig)
{
#ifdef ENABLE_OSX
	// Construct and start the osx grabber if the configuration is present
	_osxGrabber = new OsxWrapper(
			grabberConfig["display"].toInt(0),
			_grabber_width, _grabber_height, _grabber_frequency);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _osxGrabber, &OsxWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _osxGrabber, &OsxWrapper::handleSettingsUpdate);

	Info(_log, "OSX grabber created");
#else
	Error(_log, "The osx grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createCecHandler()
{
#ifdef ENABLE_CEC
	_cecHandler = new CECHandler;

	QThread * thread = new QThread(this);
	thread->setObjectName("CECThread");
	_cecHandler->moveToThread(thread);
	thread->start();

	connect(_cecHandler, &CECHandler::cecEvent, [&] (CECEvent event) {
		if (_v4l2Grabber)
			_v4l2Grabber->handleCecEvent(event);
	});

	Info(_log, "CEC handler created");
#else
	Error(_log, "The CEC handler can not be instantiated, because it has been left out from the build");
#endif
}
