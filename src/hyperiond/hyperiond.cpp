#include <unistd.h>
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

// bonjour browser
#include <bonjour/bonjourbrowserwrapper.h>

#include <jsonserver/JsonServer.h>
#include <webserver/WebServer.h>
#include <HyperionConfig.h> // Required to determine the cmake options
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

HyperionDaemon* HyperionDaemon::daemon = nullptr;

HyperionDaemon::HyperionDaemon(const QString rootPath, QObject *parent, const bool& logLvlOverwrite)
	: QObject(parent)
	, _log(Logger::getInstance("DAEMON"))
	, _instanceManager(new HyperionIManager(rootPath, this))
	, _authManager(new AuthManager(this))
	, _bonjourBrowserWrapper(new BonjourBrowserWrapper())
	, _netOrigin(new NetOrigin(this))
	, _pyInit(new PythonInit())
	, _webserver(nullptr)
	, _sslWebserver(nullptr)
	, _jsonServer(nullptr)
	, _v4l2Grabber(nullptr)
	, _dispmanx(nullptr)
	, _x11Grabber(nullptr)
	, _amlGrabber(nullptr)
	, _fbGrabber(nullptr)
	, _osxGrabber(nullptr)
	, _qtGrabber(nullptr)
	, _ssdp(nullptr)
	, _currVideoMode(VIDEO_2D)
{
	HyperionDaemon::daemon = this;

	// Register metas for thread queued connection
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");
	qRegisterMetaType<settings::type>("settings::type");
	qRegisterMetaType<VideoMode>("VideoMode");
	qRegisterMetaType<QMap<quint8,QJsonObject>>("QMap<quint8,QJsonObject>");
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");

	// init settings
	_settingsManager = new SettingsManager(0,this);

	// set inital log lvl if the loglvl wasn't overwritten by arg
	if(!logLvlOverwrite)
		handleSettingsUpdate(settings::LOGGER, getSetting(settings::LOGGER));

	// init EffectFileHandler
	EffectFileHandler* efh = new EffectFileHandler(rootPath, getSetting(settings::EFFECTS), this);
	connect(this, &HyperionDaemon::settingsChanged, efh, &EffectFileHandler::handleSettingsUpdate);

	// connect and apply settings for AuthManager
	connect(this, &HyperionDaemon::settingsChanged, _authManager, &AuthManager::handleSettingsUpdate);
	_authManager->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

	// connect and apply settings for NetOrigin
	connect(this, &HyperionDaemon::settingsChanged, _netOrigin, &NetOrigin::handleSettingsUpdate);
	_netOrigin->handleSettingsUpdate(settings::NETWORK, _settingsManager->getSetting(settings::NETWORK));

	// spawn all Hyperion instances (non blocking)
	_instanceManager->startAll();

	//connect(_hyperion,SIGNAL(closing()),this,SLOT(freeObjects())); // TODO for app restart, refactor required

	// pipe settings changes and component state changes from HyperionIManager to Daemon
	connect(_instanceManager, &HyperionIManager::settingsChanged, this, &HyperionDaemon::settingsChanged);
	connect(_instanceManager, &HyperionIManager::componentStateChanged, this, &HyperionDaemon::componentStateChanged);

	// listen for setting changes of framegrabber and v4l2
	connect(this, &HyperionDaemon::settingsChanged, this, &HyperionDaemon::handleSettingsUpdate);

	// forward videoModes from HyperionIManager to Daemon evaluation
	connect(_instanceManager, &HyperionIManager::requestVideoMode, this, &HyperionDaemon::setVideoMode);
	// return videoMode changes from Daemon to HyperionIManager
	connect(this, &HyperionDaemon::videoMode, _instanceManager, &HyperionIManager::newVideoMode);

	// ---- grabber -----
	#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB) && !defined(ENABLE_X11) && !defined(ENABLE_AMLOGIC)
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
	freeObjects();
	delete _settingsManager;
	delete _pyInit;
}

void HyperionDaemon::setVideoMode(const VideoMode& mode)
{
	if(_currVideoMode != mode)
	{
		_currVideoMode = mode;
		emit videoMode(mode);
	}
}

const QJsonDocument HyperionDaemon::getSetting(const settings::type &type)
{
	return _settingsManager->getSetting(type);
}

void HyperionDaemon::freeObjects()
{
	// destroy network first as a client might want to access hyperion
	delete _jsonServer;
	_flatBufferServer->thread()->quit();
	_flatBufferServer->thread()->wait(1000);
	_protoServer->thread()->quit();
	_protoServer->thread()->wait(1000);
	//ssdp before webserver
	_ssdp->thread()->quit();
	_ssdp->thread()->wait(1000);
	_webserver->thread()->quit();
	_webserver->thread()->wait(1000);
	_sslWebserver->thread()->quit();
	_sslWebserver->thread()->wait(1000);

	// stop Hyperions (non blocking)
	_instanceManager->stopAll();

	delete _bonjourBrowserWrapper;
	delete _amlGrabber;
	delete _dispmanx;
	delete _fbGrabber;
	delete _osxGrabber;
	delete _qtGrabber;
	delete _v4l2Grabber;

	_v4l2Grabber           = nullptr;
	_bonjourBrowserWrapper = nullptr;
	_amlGrabber            = nullptr;
	_dispmanx              = nullptr;
	_fbGrabber             = nullptr;
	_osxGrabber            = nullptr;
	_qtGrabber             = nullptr;
	_flatBufferServer      = nullptr;
	_protoServer           = nullptr;
	_ssdp                  = nullptr;
	_webserver             = nullptr;
	_sslWebserver          = nullptr;
	_jsonServer            = nullptr;
}

void HyperionDaemon::startNetworkServices()
{
	// Create Json server
	_jsonServer = new JsonServer(getSetting(settings::JSONSERVER));
	connect(this, &HyperionDaemon::settingsChanged, _jsonServer, &JsonServer::handleSettingsUpdate);

	// Create FlatBuffer server in thread
	_flatBufferServer = new FlatBufferServer(getSetting(settings::FLATBUFSERVER));
	QThread* fbThread = new QThread(this);
	_flatBufferServer->moveToThread(fbThread);
	connect( fbThread, &QThread::started, _flatBufferServer, &FlatBufferServer::initServer );
	connect( fbThread, &QThread::finished, _flatBufferServer, &QObject::deleteLater );
	connect( fbThread, &QThread::finished, fbThread, &QObject::deleteLater );
	connect(this, &HyperionDaemon::settingsChanged, _flatBufferServer, &FlatBufferServer::handleSettingsUpdate);
	fbThread->start();

	// Create Proto server in thread
	_protoServer = new ProtoServer(getSetting(settings::PROTOSERVER));
	QThread* pThread = new QThread(this);
	_protoServer->moveToThread(pThread);
	connect( pThread, &QThread::started, _protoServer, &ProtoServer::initServer );
	connect( pThread, &QThread::finished, _protoServer, &QObject::deleteLater );
	connect( pThread, &QThread::finished, pThread, &QObject::deleteLater );
	connect( this, &HyperionDaemon::settingsChanged, _protoServer, &ProtoServer::handleSettingsUpdate );
	pThread->start();

	// Create Webserver in thread
	_webserver = new WebServer(getSetting(settings::WEBSERVER), false);
	QThread* wsThread = new QThread(this);
	_webserver->moveToThread(wsThread);
	connect( wsThread, &QThread::started, _webserver, &WebServer::initServer );
	connect( wsThread, &QThread::finished, _webserver, &QObject::deleteLater );
	connect( wsThread, &QThread::finished, wsThread, &QObject::deleteLater );
	connect(this, &HyperionDaemon::settingsChanged, _webserver, &WebServer::handleSettingsUpdate);
	wsThread->start();

	// Create SSL Webserver in thread
	_sslWebserver = new WebServer(getSetting(settings::WEBSERVER), true);
	QThread* sslWsThread = new QThread(this);
	_sslWebserver->moveToThread(sslWsThread);
	connect( sslWsThread, &QThread::started, _sslWebserver, &WebServer::initServer );
	connect( sslWsThread, &QThread::finished, _sslWebserver, &QObject::deleteLater );
	connect( sslWsThread, &QThread::finished, sslWsThread, &QObject::deleteLater );
	connect(this, &HyperionDaemon::settingsChanged, _sslWebserver, &WebServer::handleSettingsUpdate);
	sslWsThread->start();

	// Create SSDP server in thread
	_ssdp = new SSDPHandler(_webserver, getSetting(settings::FLATBUFSERVER).object()["port"].toInt(), getSetting(settings::JSONSERVER).object()["port"].toInt());
	QThread* ssdpThread = new QThread(this);
	_ssdp->moveToThread(ssdpThread);
	connect( ssdpThread, &QThread::started, _ssdp, &SSDPHandler::initServer );
	connect( ssdpThread, &QThread::finished, _ssdp, &QObject::deleteLater );
	connect( ssdpThread, &QThread::finished, ssdpThread, &QObject::deleteLater );
	connect( _webserver, &WebServer::stateChange, _ssdp, &SSDPHandler::handleWebServerStateChange);
	connect(this, &HyperionDaemon::settingsChanged, _ssdp, &SSDPHandler::handleSettingsUpdate);
	ssdpThread->start();
}

void HyperionDaemon::handleSettingsUpdate(const settings::type& settingsType, const QJsonDocument& config)
{
	if(settingsType == settings::LOGGER)
	{
		const QJsonObject & logConfig = config.object();

		std::string level = logConfig["level"].toString("warn").toStdString(); // silent warn verbose debug
		if (level == "silent")       Logger::setLogLevel(Logger::OFF);
		else if (level == "warn")    Logger::setLogLevel(Logger::WARNING);
		else if (level == "verbose") Logger::setLogLevel(Logger::INFO);
		else if (level == "debug")   Logger::setLogLevel(Logger::DEBUG);
	}

	if(settingsType == settings::SYSTEMCAPTURE)
	{
		const QJsonObject & grabberConfig = config.object();

		_grabber_width     = grabberConfig["width"].toInt(96);
		_grabber_height    = grabberConfig["height"].toInt(96);
		_grabber_frequency = grabberConfig["frequency_Hz"].toInt(10);

		_grabber_cropLeft   = grabberConfig["cropLeft"].toInt(0);
		_grabber_cropRight  = grabberConfig["cropRight"].toInt(0);
		_grabber_cropTop    = grabberConfig["cropTop"].toInt(0);
		_grabber_cropBottom = grabberConfig["cropBottom"].toInt(0);

		_grabber_ge2d_mode  = grabberConfig["ge2d_mode"].toInt(0);
		_grabber_device     = grabberConfig["amlogic_grabber"].toString("amvideocap0");

		#ifdef ENABLE_OSX
			QString type = "osx";
		#else
			QString type = grabberConfig["type"].toString("auto");
		#endif

		// auto eval of type
		if ( type == "auto" )
		{
			// dispmanx -> on raspi
			if (QFile::exists("/dev/vchiq"))
			{
				type = "dispmanx";
			}
			// amlogic -> /dev/amvideo exists
			else if ( QFile::exists("/dev/amvideo") )
			{
				type = "amlogic";

					if ( !QFile::exists("/dev/" + _grabber_device) )
						{ Error( _log, "grabber device '%s' for type amlogic not found!", QSTRING_CSTR(_grabber_device)); }
			}
			// x11 -> if DISPLAY is set
			else if (getenv("DISPLAY") != NULL )
			{
				type = "x11";
			}
			// qt -> if nothing other applies
			else
			{
				type = "qt";
			}
		}

		if(_prevType != type)
		{
			Info(  _log, "set screen capture device to '%s'", QSTRING_CSTR(type));

			// stop all capture interfaces
			#ifdef ENABLE_FB
			if(_fbGrabber != nullptr)  _fbGrabber->stop();
			#endif
			#ifdef ENABLE_DISPMANX
			if(_dispmanx != nullptr)   _dispmanx->stop();
			#endif
			#ifdef ENABLE_AMLOGIC
			if(_amlGrabber != nullptr) _amlGrabber->stop();
			#endif
			#ifdef ENABLE_OSX
			if(_osxGrabber != nullptr) _osxGrabber->stop();
			#endif
			#ifdef ENABLE_X11
			if(_x11Grabber != nullptr) _x11Grabber->stop();
			#endif
			#ifdef ENABLE_QT
			if(_qtGrabber != nullptr) _qtGrabber->stop();
			#endif

			// create/start capture interface
			if(type == "framebuffer")
			{
				if(_fbGrabber == nullptr)
					createGrabberFramebuffer(grabberConfig);
				#ifdef ENABLE_FB
				_fbGrabber->start();
				#endif
			}
			else if(type == "dispmanx")
			{
				if(_dispmanx == nullptr)
					createGrabberDispmanx();
				#ifdef ENABLE_DISPMANX
				_dispmanx->start();
				#endif
			}
			else if(type == "amlogic")
			{
				if(_amlGrabber == nullptr)
					createGrabberAmlogic();
				#ifdef ENABLE_AMLOGIC
				_amlGrabber->start();
				#endif
			}
			else if(type == "osx")
			{
				if(_osxGrabber == nullptr)
					createGrabberOsx(grabberConfig);
				#ifdef ENABLE_OSX
				_osxGrabber->start();
				#endif
			}
			else if(type == "x11")
			{
				if(_x11Grabber == nullptr)
					createGrabberX11(grabberConfig);
				#ifdef ENABLE_X11
				_x11Grabber->start();
				#endif
			}
			else if(type == "qt")
			{
				if(_qtGrabber == nullptr)
					createGrabberQt(grabberConfig);
				#ifdef ENABLE_QT
				_qtGrabber->start();
				#endif
			}
			else
			{
				Error(_log,"Unknown platform capture type: %s", QSTRING_CSTR(type));
				return;
			}
			_prevType = type;
		}
	}
	else if(settingsType == settings::V4L2)
	{

#ifdef ENABLE_V4L2
			if(_v4l2Grabber != nullptr)
				return;

			const QJsonObject & grabberConfig = config.object();

			_v4l2Grabber = new V4L2Wrapper(
				grabberConfig["device"].toString("auto"),
				parseVideoStandard(grabberConfig["standard"].toString("no-change")),
				parsePixelFormat(grabberConfig["pixelFormat"].toString("no-change")),
				grabberConfig["sizeDecimation"].toInt(8) );
			_v4l2Grabber->setSignalThreshold(
				grabberConfig["redSignalThreshold"].toDouble(0.0)/100.0,
				grabberConfig["greenSignalThreshold"].toDouble(0.0)/100.0,
				grabberConfig["blueSignalThreshold"].toDouble(0.0)/100.0);
			_v4l2Grabber->setCropping(
				grabberConfig["cropLeft"].toInt(0),
				grabberConfig["cropRight"].toInt(0),
				grabberConfig["cropTop"].toInt(0),
				grabberConfig["cropBottom"].toInt(0));
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
			connect(this, &HyperionDaemon::componentStateChanged, _v4l2Grabber, &V4L2Wrapper::componentStateChanged);
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

void HyperionDaemon::createGrabberX11(const QJsonObject & grabberConfig)
{
#ifdef ENABLE_X11
	_x11Grabber = new X11Wrapper(
				_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom,
				grabberConfig["pixelDecimation"].toInt(8),
				_grabber_frequency );
	_x11Grabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _x11Grabber, &X11Wrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _x11Grabber, &X11Wrapper::handleSettingsUpdate);

	Info(_log, "X11 grabber created");
#else
	Error(_log, "The X11 grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberQt(const QJsonObject & grabberConfig)
{
#ifdef ENABLE_QT
	_qtGrabber = new QtWrapper(
				_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom,
				grabberConfig["pixelDecimation"].toInt(8),
				grabberConfig["display"].toInt(0),
				_grabber_frequency );

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _qtGrabber, &QtWrapper::setVideoMode);
	connect(this, &HyperionDaemon::settingsChanged, _qtGrabber, &QtWrapper::handleSettingsUpdate);

	Info(_log, "Qt grabber created");
#else
	Error(_log, "The Qt grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberFramebuffer(const QJsonObject & grabberConfig)
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


void HyperionDaemon::createGrabberOsx(const QJsonObject & grabberConfig)
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
