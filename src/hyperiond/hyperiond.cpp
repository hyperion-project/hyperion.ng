#include <unistd.h>
#include <cassert>
#include <stdlib.h>

#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QHostInfo>
#include <QHostAddress>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <cstdint>
#include <limits>

#include "HyperionConfig.h"

#include <utils/jsonschema/QJsonFactory.h>
#include <utils/Components.h>

#include <hyperion/Hyperion.h>
#include <hyperion/PriorityMuxer.h>
#include <effectengine/EffectEngine.h>
#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>
#include <udplistener/UDPListener.h>

#include "hyperiond.h"

HyperionDaemon::HyperionDaemon(QString configFile, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("MAIN"))
	, _kodiVideoChecker(nullptr)
	, _jsonServer(nullptr)
	, _protoServer(nullptr)
	, _boblightServer(nullptr)
	, _udpListener(nullptr)
	, _v4l2Grabbers()
	, _dispmanx(nullptr)
#ifdef ENABLE_X11
	, _x11Grabber(nullptr)
#endif
	, _amlGrabber(nullptr)
	, _fbGrabber(nullptr)
	, _osxGrabber(nullptr)
	, _hyperion(nullptr)
	, _stats(nullptr)
{
	loadConfig(configFile);

	if (Logger::getLogLevel() == Logger::WARNING)
	{
		if (_qconfig.contains("logger"))
		{
			const QJsonObject & logConfig = _qconfig["logger"].toObject();
			std::string level = logConfig["level"].toString("warn").toStdString(); // silent warn verbose debug
			if (level == "silent") Logger::setLogLevel(Logger::OFF);
			else if (level == "warn")    Logger::setLogLevel(Logger::WARNING);
			else if (level == "verbose") Logger::setLogLevel(Logger::INFO);
			else if (level == "debug")   Logger::setLogLevel(Logger::DEBUG);
			else Error(Logger::getInstance("LOGGER"), "log level '%s' used in config is unknown. valid: silent warn verbose debug", level.c_str());
			
		}
	}
	else
	{
		WarningIf(_qconfig.contains("logger"), Logger::getInstance("LOGGER"), "Logger settings overridden by command line argument");
	}
	
	_hyperion = Hyperion::initInstance(_qconfig, configFile);

	Info(_log, "Hyperion initialized");
}

HyperionDaemon::~HyperionDaemon()
{
	freeObjects();
	delete _hyperion;
}

void HyperionDaemon::freeObjects()
{
	_hyperion->clearall(true);
	Debug(_log, "destroy grabbers and network stuff");
	delete _amlGrabber;
	delete _dispmanx;
	delete _fbGrabber;
	delete _osxGrabber;
	for(V4L2Wrapper* grabber : _v4l2Grabbers)
	{
		delete grabber;
	}
	delete _kodiVideoChecker;
	delete _jsonServer;
	delete _protoServer;
	delete _boblightServer;
	delete _udpListener;
	delete _stats;

	_v4l2Grabbers.clear();
	_amlGrabber     = nullptr;
	_dispmanx       = nullptr;
	_fbGrabber      = nullptr;
	_osxGrabber     = nullptr;
	_kodiVideoChecker = nullptr;
	_jsonServer     = nullptr;
	_protoServer    = nullptr;
	_boblightServer = nullptr;
	_udpListener    = nullptr;
	_stats          = nullptr;
}

void HyperionDaemon::run()
{
	createKODIVideoChecker();

	// ---- network services -----
	startNetworkServices();

	// ---- grabber -----
	createGrabberV4L2();
	createSystemFrameGrabber();

	#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB) && !defined(ENABLE_X11) && !defined(ENABLE_AMLOGIC)
		WarningIf(_qconfig.contains("framegrabber"), _log, "No grabber can be instantiated, because all grabbers have been left out from the build");
	#endif
	Info(_log, "Hyperion started");

	connect(_hyperion,SIGNAL(closing()),this,SLOT(freeObjects()));

	startInitialEffect();
}

void HyperionDaemon::loadConfig(const QString & configFile)
{
	Info(_log, "Selected configuration file: %s", QSTRING_CSTR(configFile));
	
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	// read the json schema from the resource
	
	QString schemaFile = ":/hyperion-schema";
	QJsonObject schemaJson;
	try
	{
		schemaJson = QJsonFactory::readSchema(schemaFile);
	}
	catch(const std::runtime_error& error)
	{
		throw std::runtime_error(error.what());
	}

	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);

	_qconfig = QJsonFactory::readConfig(configFile);
	QPair<bool, bool> validate = schemaChecker.validate(_qconfig);
	
	if (!validate.first && validate.second)
	{
		Warning(_log,"Errors have been found in the configuration file. Automatic correction is applied");
		
		_qconfig = schemaChecker.getAutoCorrectedConfig(_qconfig);

		if (!QJsonFactory::writeJson(configFile, _qconfig))
			throw std::runtime_error("ERROR: can not save configuration file, aborting ");
	}
	else if (validate.first && !validate.second) //Error in Schema
	{
		QStringList schemaErrors = schemaChecker.getMessages();
		foreach (auto & schemaError, schemaErrors)
			std::cout << schemaError.toStdString() << std::endl;

		throw std::runtime_error("ERROR: Json validation failed");
	}
}


void HyperionDaemon::startInitialEffect()
{
	#define FGCONFIG_ARRAY fgColorConfig.toArray()
	#define BGCONFIG_ARRAY bgColorConfig.toArray()

	Hyperion *hyperion = Hyperion::getInstance();

	// create boot sequence
		const QJsonObject & FGEffectConfig = _qconfig["foregroundEffect"].toObject();
		const QJsonObject & BGEffectConfig = _qconfig["backgroundEffect"].toObject();
		const int FG_PRIORITY = 0;
		const int DURATION_INFINITY = 0;
		const int BG_PRIORITY = PriorityMuxer::LOWEST_PRIORITY-1;

	// clear the leds
	hyperion->setColor(FG_PRIORITY, ColorRgb::BLACK, 100, false);

	// initial foreground effect/color
	if (FGEffectConfig["enable"].toBool(true))
	{
		const QString fgTypeConfig = FGEffectConfig["type"].toString("effect");
		const QString fgEffectConfig = FGEffectConfig["effect"].toString("Rainbow swirl fast");
		const QJsonValue fgColorConfig = FGEffectConfig["color"];
		int default_fg_duration_ms = 3000;
		int fg_duration_ms = FGEffectConfig["duration_ms"].toInt(default_fg_duration_ms);
		if (fg_duration_ms == DURATION_INFINITY)
		{
			fg_duration_ms = default_fg_duration_ms;
			Warning(_log, "foreground effect duration 'infinity' is forbidden, set to default value %d ms",default_fg_duration_ms);
		}
		if ( fgTypeConfig.contains("color") )
		{
			ColorRgb fg_color = {
				(uint8_t)FGCONFIG_ARRAY.at(0).toInt(0),
				(uint8_t)FGCONFIG_ARRAY.at(1).toInt(0),
				(uint8_t)FGCONFIG_ARRAY.at(2).toInt(0)
			};
			hyperion->setColor(FG_PRIORITY, fg_color, fg_duration_ms, false);
			Info(_log,"Inital foreground color set (%d %d %d)",fg_color.red,fg_color.green,fg_color.blue);
		}
		else
		{
			int result = hyperion->setEffect(fgEffectConfig, FG_PRIORITY, fg_duration_ms);
			Info(_log,"Inital foreground effect '%s' %s", QSTRING_CSTR(fgEffectConfig), ((result == 0) ? "started" : "failed"));
		}
	}
	// initial background effect/color
	if (BGEffectConfig["enable"].toBool(true))
	{
		const QString bgTypeConfig = BGEffectConfig["type"].toString("effect");
		const QString bgEffectConfig = BGEffectConfig["effect"].toString("Warm mood blobs");
		const QJsonValue bgColorConfig = BGEffectConfig["color"];
		if (bgTypeConfig.contains("color"))
		{
			ColorRgb bg_color = {
				(uint8_t)BGCONFIG_ARRAY.at(0).toInt(0),
				(uint8_t)BGCONFIG_ARRAY.at(1).toInt(0),
				(uint8_t)BGCONFIG_ARRAY.at(2).toInt(0)
			};
			hyperion->setColor(BG_PRIORITY, bg_color, DURATION_INFINITY, false);
			Info(_log,"Inital background color set (%d %d %d)",bg_color.red,bg_color.green,bg_color.blue);
		}
		else
		{
			int result = hyperion->setEffect(bgEffectConfig, BG_PRIORITY, DURATION_INFINITY);
			Info(_log,"Inital background effect '%s' %s", QSTRING_CSTR(bgEffectConfig), ((result == 0) ? "started" : "failed"));
		}
	}
	
	#undef FGCONFIG_ARRAY
	#undef BGCONFIG_ARRAY
}


// create KODI video checker if the _configuration is present
void HyperionDaemon::createKODIVideoChecker()
{
	bool kodiCheckerConfigured = _qconfig.contains("kodiVideoChecker");

	const QJsonObject & videoCheckerConfig = _qconfig["kodiVideoChecker"].toObject();
	_kodiVideoChecker = KODIVideoChecker::initInstance(
		videoCheckerConfig["kodiAddress"].toString("127.0.0.1"),
		videoCheckerConfig["kodiTcpPort"].toInt(9090),
		videoCheckerConfig["grabVideo"].toBool(true),
		videoCheckerConfig["grabPictures"].toBool(true),
		videoCheckerConfig["grabAudio"].toBool(true),
		videoCheckerConfig["grabMenu"].toBool(false),
		videoCheckerConfig["grabPause"].toBool(true),
		videoCheckerConfig["grabScreensaver"].toBool(false),
		videoCheckerConfig["enable3DDetection"].toBool(true));
	Debug(_log, "KODI checker created ");

	if( kodiCheckerConfigured && videoCheckerConfig["enable"].toBool(true))
	{
		_kodiVideoChecker->start();
	}
	_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_KODICHECKER, _kodiVideoChecker->componentState());
	QObject::connect( _hyperion, SIGNAL(componentStateChanged(hyperion::Components,bool)), _kodiVideoChecker, SLOT(componentStateChanged(hyperion::Components,bool)));
	QObject::connect(_kodiVideoChecker, SIGNAL(videoMode(VideoMode)), _hyperion, SLOT(setVideoMode(VideoMode)));
	QObject::connect(_kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _hyperion, SLOT(setGrabbingMode(GrabbingMode)));
}

void HyperionDaemon::startNetworkServices()
{
	KODIVideoChecker* kodiVideoChecker = KODIVideoChecker::getInstance();

	// Create Stats
	_stats = new Stats();

	// Create Json server if configuration is present
	unsigned int jsonPort = 19444;
	if (_qconfig.contains("jsonServer"))
	{
		const QJsonObject & jsonServerConfig = _qconfig["jsonServer"].toObject();
		//jsonEnable = jsonServerConfig.get("enable", true).asBool();
		jsonPort   = jsonServerConfig["port"].toInt(jsonPort);
	}

	_jsonServer = new JsonServer(jsonPort);
	Info(_log, "Json server created and started on port %d", _jsonServer->getPort());

	// Create Proto server if configuration is present
	unsigned int protoPort = 19445;
	if (_qconfig.contains("protoServer"))
	{
		const QJsonObject & protoServerConfig = _qconfig["protoServer"].toObject();
		//protoEnable = protoServerConfig.get("enable", true).asBool();
		protoPort  = protoServerConfig["port"].toInt(protoPort);
	}

	_protoServer = new ProtoServer(protoPort);
	if (kodiVideoChecker != nullptr)
	{
		QObject::connect(kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _protoServer, SIGNAL(grabbingMode(GrabbingMode)));
		QObject::connect(kodiVideoChecker, SIGNAL(videoMode(VideoMode)), _protoServer, SIGNAL(videoMode(VideoMode)));
	}
	Info(_log, "Proto server created and started on port %d", _protoServer->getPort());

	// Create Boblight server if configuration is present
	bool boblightConfigured = _qconfig.contains("boblightServer");

	const QJsonObject & boblightServerConfig = _qconfig["boblightServer"].toObject();
	_boblightServer = new BoblightServer(
		boblightServerConfig["priority"].toInt(710),
		boblightServerConfig["port"].toInt(19333) );
	Debug(_log, "Boblight server created");

	if ( boblightConfigured && boblightServerConfig["enable"].toBool(true))
	{
		_boblightServer->start();
	}
	_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_BOBLIGHTSERVER, _boblightServer->componentState());
	connect( Hyperion::getInstance(), SIGNAL(componentStateChanged(hyperion::Components,bool)), _boblightServer, SLOT(componentStateChanged(hyperion::Components,bool)));

	// Create UDP listener if configuration is present
	bool udpListenerConfigured = _qconfig.contains("udpListener");
	const QJsonObject & udpListenerConfig = _qconfig["udpListener"].toObject();
	_udpListener = new UDPListener(
				udpListenerConfig["priority"].toInt(700),
				udpListenerConfig["timeout"].toInt(10000),
				udpListenerConfig["address"].toString(""),
				udpListenerConfig["port"].toInt(2801),
				udpListenerConfig["shared"].toBool(false));

	Debug(_log, "UDP listener created");

	if ( udpListenerConfigured && udpListenerConfig["enable"].toBool(true))
	{
		_udpListener->start();
	}
	_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_UDPLISTENER, _udpListener->componentState());
	connect( Hyperion::getInstance(), SIGNAL(componentStateChanged(hyperion::Components,bool)), _udpListener, SLOT(componentStateChanged(hyperion::Components,bool)));

	// zeroconf description - $leddevicename@$hostname
	const QJsonObject & generalConfig = _qconfig["general"].toObject();
	const QString mDNSDescr = generalConfig["name"].toString("") + "@" + QHostInfo::localHostName();
	// txt record for zeroconf
	QString id = _hyperion->id;
	std::string version = HYPERION_VERSION;
	std::vector<std::pair<std::string, std::string> > txtRecord = {{"id",id.toStdString()},{"version",version}};

	// zeroconf udp listener
	if (_udpListener != nullptr)
	{
		BonjourServiceRegister *bonjourRegister_udp = new BonjourServiceRegister();
		bonjourRegister_udp->registerService(
			BonjourRecord(mDNSDescr + ":" + QString::number(_udpListener->getPort()), "_hyperiond-udp._udp", QString()), _udpListener->getPort(), txtRecord);
		Debug(_log, "UDP LIstener mDNS responder started");
	}

	// zeroconf json
	BonjourServiceRegister *bonjourRegister_json = new BonjourServiceRegister();
	bonjourRegister_json->registerService(
		BonjourRecord(mDNSDescr + ":" + QString::number(_jsonServer->getPort()), "_hyperiond-json._tcp", QString()), _jsonServer->getPort(), txtRecord);
	Debug(_log, "Json mDNS responder started");

	// zeroconf proto
	BonjourServiceRegister *bonjourRegister_proto = new BonjourServiceRegister();
	bonjourRegister_proto->registerService(
		BonjourRecord(mDNSDescr + ":" + QString::number(_jsonServer->getPort()), "_hyperiond-proto._tcp", QString()), _protoServer->getPort(), txtRecord);
	Debug(_log, "Proto mDNS responder started");
}


void HyperionDaemon::createSystemFrameGrabber()
{
	if (_qconfig.contains("framegrabber"))
	{
		const QJsonObject & grabberConfig = _qconfig["framegrabber"].toObject();
// 		if (grabberConfig["enable"].toBool(true))
		{
			_grabber_width     = grabberConfig["width"].toInt(96);
			_grabber_height    = grabberConfig["height"].toInt(96);
			_grabber_frequency = grabberConfig["frequency_Hz"].toInt(10);
			_grabber_priority  = grabberConfig["priority"].toInt(900);

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
				// TODO currently a compile option
				#ifdef ENABLE_DISPMANX
				if (true)
				#else
				if (false)
				#endif
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
			
			bool grabberCompState = grabberConfig["enable"].toBool(true);
			if (type == "") { Info( _log, "screen capture device disabled"); grabberCompState = false; }
			else if (type == "framebuffer")   createGrabberFramebuffer(grabberConfig);
			else if (type == "dispmanx") createGrabberDispmanx();
			else if (type == "amlogic")  createGrabberAmlogic();
			else if (type == "osx")      createGrabberOsx(grabberConfig);
			else if (type == "x11")      createGrabberX11(grabberConfig);
			else { Warning( _log, "unknown framegrabber type '%s'", QSTRING_CSTR(type)); grabberCompState = false; }
			
//			_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_GRABBER, grabberCompState);
			_hyperion->setComponentState(hyperion::COMP_GRABBER, grabberCompState );
		}
	}
}


void HyperionDaemon::createGrabberDispmanx()
{
#ifdef ENABLE_DISPMANX
	_dispmanx = new DispmanxWrapper(_grabber_width, _grabber_height, _grabber_frequency, _grabber_priority);
	_dispmanx->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	QObject::connect(_kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _dispmanx, SLOT(setGrabbingMode(GrabbingMode)));
	QObject::connect(_hyperion, SIGNAL(videoMode(VideoMode)), _dispmanx, SLOT(setVideoMode(VideoMode)));
	QObject::connect(_dispmanx, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
	QObject::connect(_dispmanx, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _hyperion, SLOT(setImage(int, const Image<ColorRgb>&, const int)) );

	_dispmanx->start();

	Info(_log, "DISPMANX frame grabber created and started");
#else
	ErrorIf(_qconfig.contains("framegrabber"), _log, "The dispmanx framegrabber can not be instantiated, because it has been left out from the build");
#endif
}


void HyperionDaemon::createGrabberAmlogic()
{
#ifdef ENABLE_AMLOGIC
	_amlGrabber = new AmlogicWrapper(_grabber_width, _grabber_height, _grabber_frequency, _grabber_priority);
	_amlGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	QObject::connect(_amlGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

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
				grabberConfig["useXGetImage"].toBool(false),
				_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom,
				grabberConfig["horizontalPixelDecimation"].toInt(8),
				grabberConfig["verticalPixelDecimation"].toInt(8),
				_grabber_frequency, _grabber_priority );
	_x11Grabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);

	QObject::connect(_x11Grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

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
				_grabber_width, _grabber_height, _grabber_frequency, _grabber_priority);
	_fbGrabber->setCropping(_grabber_cropLeft, _grabber_cropRight, _grabber_cropTop, _grabber_cropBottom);
	QObject::connect(_fbGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

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
				_grabber_width, _grabber_height, _grabber_frequency, _grabber_priority);
	
	QObject::connect(_osxGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

	_osxGrabber->start();
	Info(_log, "OSX grabber created and started");
#else
	Error(_log, "The osx grabber can not be instantiated, because it has been left out from the build");
#endif
}


void HyperionDaemon::createGrabberV4L2()
{
	// construct and start the v4l2 grabber if the configuration is present
	bool v4lConfigured = _qconfig.contains("grabberV4L2");
	bool v4lStarted = false;
	unsigned v4lEnableCount = 0;
	
	if (_qconfig["grabberV4L2"].isArray())
	{
		const QJsonArray & v4lArray = _qconfig["grabberV4L2"].toArray();
		for ( signed idx=0; idx<v4lArray.size(); idx++)
		{
			const QJsonObject & grabberConfig = v4lArray.at(idx).toObject();
			bool enableV4l = v4lConfigured && grabberConfig["enable"].toBool(true);
			if (enableV4l)
			{
				v4lEnableCount++;
			}
			#ifdef ENABLE_V4L2
			V4L2Wrapper* grabber = new V4L2Wrapper(
				grabberConfig["device"].toString("auto"),
				grabberConfig["input"].toInt(0),
				parseVideoStandard(grabberConfig["standard"].toString("no-change")),
				parsePixelFormat(grabberConfig["pixelFormat"].toString("no-change")),
				grabberConfig["width"].toInt(0),
				grabberConfig["height"].toInt(0),
				grabberConfig["frameDecimation"].toInt(2),
				grabberConfig["sizeDecimation"].toInt(8),
				grabberConfig["redSignalThreshold"].toDouble(0.0)/100.0,
				grabberConfig["greenSignalThreshold"].toDouble(0.0)/100.0,
				grabberConfig["blueSignalThreshold"].toDouble(0.0)/100.0,
				grabberConfig["priority"].toInt(890),
				grabberConfig["useKodiChecker"].toBool(false));
			grabber->setVideoMode(parse3DMode(grabberConfig["mode"].toString("2D")));
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

			QObject::connect(grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)));

			if (enableV4l && grabber->start())
			{
				v4lStarted = true;
				Info(_log, "V4L2 grabber started");
			}
			_v4l2Grabbers.push_back(grabber);
			#endif
		}
	}

	ErrorIf( (v4lEnableCount>0 && _v4l2Grabbers.size()==0), _log, "The v4l2 grabber can not be instantiated, because it has been left out from the build");
	_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_V4L, (_v4l2Grabbers.size()>0 && v4lEnableCount>0 && v4lStarted) );
}
