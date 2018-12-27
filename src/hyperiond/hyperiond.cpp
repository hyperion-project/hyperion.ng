#include <unistd.h>
#include <cassert>
#include <stdlib.h>

#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QHostAddress>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <cstdint>
#include <limits>

#include <utils/Components.h>
#include <utils/JsonUtils.h>

#include <hyperion/Hyperion.h>
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>
#include <udplistener/UDPListener.h>
#include <webserver/WebServer.h>
#include <utils/Stats.h>
#include <HyperionConfig.h> // Required to determine the cmake options
#include "hyperiond.h"

// bonjour browser
#include <bonjour/bonjourbrowserwrapper.h>

// settings
#include <hyperion/SettingsManager.h>

// Init Python
#include <python/PythonInit.h>

HyperionDaemon* HyperionDaemon::daemon = nullptr;

HyperionDaemon::HyperionDaemon(QString configFile, const QString rootPath, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("DAEMON"))
	, _bonjourBrowserWrapper(new BonjourBrowserWrapper())
	, _pyInit(new PythonInit())
	, _webserver(nullptr)
	, _jsonServer(nullptr)
	, _protoServer(nullptr)
	, _boblightServer(nullptr)
	, _udpListener(nullptr)
	, _v4l2Grabbers()
	, _dispmanx(nullptr)
	, _x11Grabber(nullptr)
	, _amlGrabber(nullptr)
	, _fbGrabber(nullptr)
	, _osxGrabber(nullptr)
	, _hyperion(nullptr)
	, _stats(nullptr)
	, _currVideoMode(VIDEO_2D)
{
	HyperionDaemon::daemon = this;

	// init settings
	_settingsManager = new SettingsManager(0,configFile);

	const QJsonObject& logConfig = _settingsManager->getSetting(settings::LOGGER).object();
	if (Logger::getLogLevel() == Logger::WARNING)
	{
		std::string level = logConfig["level"].toString("warn").toStdString(); // silent warn verbose debug
		if (level == "silent")       Logger::setLogLevel(Logger::OFF);
		else if (level == "warn")    Logger::setLogLevel(Logger::WARNING);
		else if (level == "verbose") Logger::setLogLevel(Logger::INFO);
		else if (level == "debug")   Logger::setLogLevel(Logger::DEBUG);
	}
	else
	{
		Warning(Logger::getInstance("LOGGER"), "Logger settings overridden by command line argument");
	}

	_hyperion = Hyperion::initInstance(this, 0, configFile, rootPath);

	Info(_log, "Hyperion initialized");

	connect(_hyperion,SIGNAL(closing()),this,SLOT(freeObjects()));
	// listen for setting changes
	connect(_hyperion, &Hyperion::settingsChanged, this, &HyperionDaemon::settingsChanged);
	// listen for setting changes of framegrabber and v4l2
	connect(this, &HyperionDaemon::settingsChanged, this, &HyperionDaemon::handleSettingsUpdate);
	// forward system and v4l images to Hyperion
	connect(this, &HyperionDaemon::systemImage, _hyperion, &Hyperion::systemImage);
	connect(this, &HyperionDaemon::v4lImage, _hyperion, &Hyperion::v4lImage);
	// forward videoModes from Hyperion to Daemon evaluation
	connect(_hyperion, &Hyperion::videoMode, this, &HyperionDaemon::setVideoMode);
	// forward videoMode changes from Daemon to Hyperion
	connect(this, &HyperionDaemon::videoMode, _hyperion, &Hyperion::newVideoMode);

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
	delete _hyperion;
	delete _settingsManager;
	delete _pyInit;
}

quint16 HyperionDaemon::getWebServerPort()
{
	return _webserver->getPort();
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
	_hyperion->clearall(true);
	// destroy network first as a client might want to access pointers
	delete _webserver;
	delete _jsonServer;
	delete _protoServer;
	delete _boblightServer;
	delete _udpListener;

	delete _bonjourBrowserWrapper;
	delete _amlGrabber;
	delete _dispmanx;
	delete _fbGrabber;
	delete _osxGrabber;
	for(V4L2Wrapper* grabber : _v4l2Grabbers)
	{
		delete grabber;
	}
	delete _stats;

	_v4l2Grabbers.clear();
	_bonjourBrowserWrapper = nullptr;
	_amlGrabber     = nullptr;
	_dispmanx       = nullptr;
	_fbGrabber      = nullptr;
	_osxGrabber     = nullptr;
	_webserver      = nullptr;
	_jsonServer     = nullptr;
	_protoServer    = nullptr;
	_boblightServer = nullptr;
	_udpListener    = nullptr;
	_stats          = nullptr;
}

void HyperionDaemon::startNetworkServices()
{
	// Create Stats before network services
	_stats = new Stats();

	// Create Json server
	_jsonServer = new JsonServer(getSetting(settings::JSONSERVER));
	connect(this, &HyperionDaemon::settingsChanged, _jsonServer, &JsonServer::handleSettingsUpdate);

	// Create Proto server
	_protoServer = new ProtoServer(getSetting(settings::PROTOSERVER));
	connect(this, &HyperionDaemon::settingsChanged, _protoServer, &ProtoServer::handleSettingsUpdate);
	//QObject::connect(_hyperion, SIGNAL(videoMode(VideoMode)), _protoServer, SLOT(setVideoMode(VideoMode)));

	// boblight server
	_boblightServer = new BoblightServer(getSetting(settings::BOBLSERVER));
	connect(this, &HyperionDaemon::settingsChanged, _boblightServer, &BoblightServer::handleSettingsUpdate);

	// Create UDP listener
	_udpListener = new UDPListener(getSetting(settings::UDPLISTENER));
	connect(this, &HyperionDaemon::settingsChanged, _udpListener, &UDPListener::handleSettingsUpdate);

	// Create Webserver
	_webserver = new WebServer(getSetting(settings::WEBSERVER));
	connect(this, &HyperionDaemon::settingsChanged, _webserver, &WebServer::handleSettingsUpdate);
}

void HyperionDaemon::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::SYSTEMCAPTURE)
	{
		const QJsonObject & grabberConfig = config.object();

		_grabber_width     = grabberConfig["width"].toInt(96);
		_grabber_height    = grabberConfig["height"].toInt(96);
		_grabber_frequency = grabberConfig["frequency_Hz"].toInt(10);

		_grabber_cropLeft   = grabberConfig["cropLeft"].toInt(0);
		_grabber_cropRight  = grabberConfig["cropRight"].toInt(0);
		_grabber_cropTop    = grabberConfig["cropTop"].toInt(0);
		_grabber_cropBottom = grabberConfig["cropBottom"].toInt(0);

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
			else if ( QFile::exists("/dev/amvideo") && ( QFile::exists("/dev/amvideocap0") || QFile::exists("/dev/ge2d") ) )
			{
				type = "amlogic";
			}
			// x11 -> if DISPLAY is set
			else if (getenv("DISPLAY") != NULL )
			{
				type = "x11";
			}
			// framebuffer -> if nothing other applies
			else
			{
				type = "framebuffer";
			}
			Info(  _log, "set screen capture device to '%s'", QSTRING_CSTR(type));
		}

		if (type == "") { Info( _log, "screen capture device disabled"); }
		else if (type == "framebuffer" && _fbGrabber == nullptr)   createGrabberFramebuffer(grabberConfig);
		else if (type == "dispmanx" && _dispmanx == nullptr)       createGrabberDispmanx();
		else if (type == "amlogic" && _amlGrabber == nullptr)      createGrabberAmlogic();
		else if (type == "osx" && _osxGrabber == nullptr)          createGrabberOsx(grabberConfig);
		else if (type == "x11" && _x11Grabber == nullptr)          createGrabberX11(grabberConfig);
		else { Warning( _log, "unknown framegrabber type '%s'", QSTRING_CSTR(type)); }
	}
	else if(type == settings::V4L2)
	{
		// stop
		if(_v4l2Grabbers.size()>0)
			return;

		unsigned v4lEnableCount = 0;

		const QJsonArray & v4lArray = config.array();
		for ( signed idx=0; idx<v4lArray.size(); idx++)
		{
			const QJsonObject & grabberConfig = v4lArray.at(idx).toObject();


			#ifdef ENABLE_V4L2
			V4L2Wrapper* grabber = new V4L2Wrapper(
				grabberConfig["device"].toString("auto"),
				grabberConfig["input"].toInt(0),
				parseVideoStandard(grabberConfig["standard"].toString("no-change")),
				parsePixelFormat(grabberConfig["pixelFormat"].toString("no-change")),
				grabberConfig["sizeDecimation"].toInt(8) );
			grabber->setSignalThreshold(
				grabberConfig["redSignalThreshold"].toDouble(0.0)/100.0,
				grabberConfig["greenSignalThreshold"].toDouble(0.0)/100.0,
				grabberConfig["blueSignalThreshold"].toDouble(0.0)/100.0);
			grabber->setCropping(
				grabberConfig["cropLeft"].toInt(0),
				grabberConfig["cropRight"].toInt(0),
				grabberConfig["cropTop"].toInt(0),
				grabberConfig["cropBottom"].toInt(0));
			grabber->setSignalDetectionEnable(grabberConfig["signalDetection"].toBool(true));
			grabber->setSignalDetectionOffset(
				grabberConfig["sDHOffsetMin"].toDouble(0.25),
				grabberConfig["sDVOffsetMin"].toDouble(0.25),
				grabberConfig["sDHOffsetMax"].toDouble(0.75),
				grabberConfig["sDVOffsetMax"].toDouble(0.75));
			Debug(_log, "V4L2 grabber created");

			// connect to HyperionDaemon signal
			connect(grabber, &V4L2Wrapper::systemImage, this, &HyperionDaemon::v4lImage);
			connect(this, &HyperionDaemon::videoMode, grabber, &V4L2Wrapper::setVideoMode);
			connect(this, &HyperionDaemon::settingsChanged, grabber, &V4L2Wrapper::handleSettingsUpdate);

			if (grabber->start())
			{
				Info(_log, "V4L2 grabber started");
			}
			_v4l2Grabbers.push_back(grabber);
			#endif
		}

		ErrorIf( (v4lEnableCount>0 && _v4l2Grabbers.size()==0), _log, "The v4l2 grabber can not be instantiated, because it has been left out from the build");
	}
}

void HyperionDaemon::createGrabberDispmanx()
{
#ifdef ENABLE_DISPMANX
	_dispmanx = new DispmanxWrapper(_grabber_width, _grabber_height, _grabber_frequency);
	_dispmanx->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _dispmanx, &DispmanxWrapper::setVideoMode);
	connect(_dispmanx, &DispmanxWrapper::systemImage, this, &HyperionDaemon::systemImage);
	connect(this, &HyperionDaemon::settingsChanged, _dispmanx, &DispmanxWrapper::handleSettingsUpdate);

	_dispmanx->start();

	Info(_log, "DISPMANX frame grabber created and started");
#else
	Error( _log, "The dispmanx framegrabber can not be instantiated, because it has been left out from the build");
#endif
}


void HyperionDaemon::createGrabberAmlogic()
{
#ifdef ENABLE_AMLOGIC
	_amlGrabber = new AmlogicWrapper(_grabber_width, _grabber_height, _grabber_frequency);
	_amlGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	// connect to HyperionDaemon signal
	connect(this, &HyperionDaemon::videoMode, _amlGrabber, &AmlogicWrapper::setVideoMode);
	connect(_amlGrabber, &AmlogicWrapper::systemImage, this, &HyperionDaemon::systemImage);
	connect(this, &HyperionDaemon::settingsChanged, _amlGrabber, &AmlogicWrapper::handleSettingsUpdate);

	_amlGrabber->start();
	Info(_log, "AMLOGIC grabber created and started");
#else
	Error( _log, "The AMLOGIC grabber can not be instantiated, because it has been left out from the build");
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
	connect(_x11Grabber, &X11Wrapper::systemImage, this, &HyperionDaemon::systemImage);
	connect(this, &HyperionDaemon::settingsChanged, _x11Grabber, &X11Wrapper::handleSettingsUpdate);

	_x11Grabber->start();
	Info(_log, "X11 grabber created and started");
#else
	Error(_log, "The X11 grabber can not be instantiated, because it has been left out from the build");
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
	connect(_fbGrabber, &FramebufferWrapper::systemImage, this, &HyperionDaemon::systemImage);
	connect(this, &HyperionDaemon::settingsChanged, _fbGrabber, &FramebufferWrapper::handleSettingsUpdate);

	_fbGrabber->start();
	Info(_log, "Framebuffer grabber created and started");
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
	connect(_osxGrabber, &OsxWrapper::systemImage, this, &HyperionDaemon::systemImage);
	connect(this, &HyperionDaemon::settingsChanged, _osxGrabber, &OsxWrapper::handleSettingsUpdate);

	_osxGrabber->start();
	Info(_log, "OSX grabber created and started");
#else
	Error(_log, "The osx grabber can not be instantiated, because it has been left out from the build");
#endif
}
