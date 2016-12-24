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
#include "configMigrator.h"

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
{
	loadConfig(configFile, CURRENT_CONFIG_VERSION );

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
	delete _hyperion;

}

void HyperionDaemon::run()
{
	startInitialEffect();
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


}

int HyperionDaemon::tryLoadConfig(const QString & configFile, const int schemaVersion)
{
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);
	QJsonParseError error;

	// read the json schema from the resource
	QString schemaFile = ":/hyperion-schema";
	if (schemaVersion > 0)
		schemaFile += "-" + QString::number(schemaVersion); 
	QFile schemaData(schemaFile);
	if (!schemaData.open(QIODevice::ReadOnly))
	{
		std::stringstream error;
		error << "Schema not found or not supported: " << schemaData.errorString().toStdString();
		throw std::runtime_error(error.str());
	}
	
	QByteArray schema = schemaData.readAll();
	QJsonDocument schemaJson = QJsonDocument::fromJson(schema, &error);
	schemaData.close();
	
	if (error.error != QJsonParseError::NoError)
	{
		// report to the user the failure and their locations in the document.
		int errorLine(0), errorColumn(0);
		
		for( int i=0, count=qMin( error.offset,schema.size()); i<count; ++i )
		{
			++errorColumn;
			if(schema.at(i) == '\n' )
			{
				errorColumn = 0;
				++errorLine;
			}
		}
		
		std::stringstream sstream;
		sstream << "ERROR: Json schema wrong: " << error.errorString().toStdString() << " at Line: " << errorLine << ", Column: " << errorColumn;

		throw std::runtime_error(sstream.str());
	}

	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson.object());

	_qconfig = QJsonFactory::readJson(configFile);
	if (!schemaChecker.validate(_qconfig))
	{
		for (std::list<std::string>::const_iterator i = schemaChecker.getMessages().begin(); i != schemaChecker.getMessages().end(); ++i)
		{
			std::cout << *i << std::endl;
		}

		throw std::runtime_error("ERROR: Json validation failed");
	}
	
	const QJsonObject & generalConfig = _qconfig["general"].toObject();
	return generalConfig["configVersion"].toInt(-1);
}


void HyperionDaemon::loadConfig(const QString & configFile, const int neededConfigVersion)
{
	Info(_log, "Selected configuration file: %s", configFile.toUtf8().constData());

	int configVersionId = tryLoadConfig(configFile,0);

	// no config id found, assume legacy hyperion
	if (configVersionId < 0)
	{
		Debug(_log, "config file has no version, assume old hyperion.");
		configVersionId = tryLoadConfig(configFile,1);
	}
	Debug(_log, "config version: %d", configVersionId);
	configVersionId = tryLoadConfig(configFile, configVersionId);

	if (neededConfigVersion == configVersionId)
	{
		return;
	}

	// migrate configVersionId
	ConfigMigrator migrator;
	migrator.migrate(configFile, configVersionId, neededConfigVersion);
	
}


void HyperionDaemon::startInitialEffect()
{
	#define FGCONFIG_ARRAY fgEffectConfig.toArray()
	#define BGCONFIG_ARRAY bgEffectConfig.toArray()

	Hyperion *hyperion = Hyperion::getInstance();

	// create boot sequence if the configuration is present
	if (_qconfig.contains("initialEffect"))
	{
		const QJsonObject & effectConfig = _qconfig["initialEffect"].toObject();
		const int FG_PRIORITY = 0;
		const int DURATION_INFINITY = 0;
		const int BG_PRIORITY = PriorityMuxer::LOWEST_PRIORITY -1;

		// clear the leds
		hyperion->setColor(FG_PRIORITY, ColorRgb::BLACK, 100, false);

		// initial foreground effect/color
		const QJsonValue fgEffectConfig = effectConfig["foreground-effect"];
		int default_fg_duration_ms = 3000;
		int fg_duration_ms = effectConfig["foreground-duration_ms"].toInt(default_fg_duration_ms);
		if (fg_duration_ms == DURATION_INFINITY)
		{
			fg_duration_ms = default_fg_duration_ms;
			Warning(_log, "foreground effect duration 'infinity' is forbidden, set to default value %d ms",default_fg_duration_ms);
		}
		if ( ! fgEffectConfig.isNull() && fgEffectConfig.isArray() && FGCONFIG_ARRAY.size() == 3 )
		{
			ColorRgb fg_color = {
				(uint8_t)FGCONFIG_ARRAY.at(0).toInt(0),
				(uint8_t)FGCONFIG_ARRAY.at(1).toInt(0),
				(uint8_t)FGCONFIG_ARRAY.at(2).toInt(0)
			};
			hyperion->setColor(FG_PRIORITY, fg_color, fg_duration_ms, false);
			Info(_log,"Inital foreground color set (%d %d %d)",fg_color.red,fg_color.green,fg_color.blue);
		}
		else if (! fgEffectConfig.isNull() && fgEffectConfig.isArray() && FGCONFIG_ARRAY.size() == 1 && FGCONFIG_ARRAY.at(0).isString())
		{
			const QString fgEffectName = FGCONFIG_ARRAY.at(0).toString();
			int result = effectConfig.contains("foreground-effect-args")
//			           ? hyperion->setEffect(fgEffectName, effectConfig["foreground-effect-args"], FG_PRIORITY, fg_duration_ms)
			           ? hyperion->setEffect(fgEffectName, _qconfig["initialEffect"].toObject()["foreground-effect-args"].toObject(), FG_PRIORITY, fg_duration_ms)
			           : hyperion->setEffect(fgEffectName, FG_PRIORITY, fg_duration_ms);
			Info(_log,"Inital foreground effect '%s' %s", fgEffectName.toUtf8().constData(), ((result == 0) ? "started" : "failed"));
		}

		// initial background effect/color
		const QJsonValue bgEffectConfig = effectConfig["background-effect"];
		if ( ! bgEffectConfig.isNull() && bgEffectConfig.isArray() && BGCONFIG_ARRAY.size() == 3 )
		{
			ColorRgb bg_color = {
				(uint8_t)BGCONFIG_ARRAY.at(0).toInt(0),
				(uint8_t)BGCONFIG_ARRAY.at(1).toInt(0),
				(uint8_t)BGCONFIG_ARRAY.at(2).toInt(0)
			};
			hyperion->setColor(BG_PRIORITY, bg_color, DURATION_INFINITY, false);
			Info(_log,"Inital background color set (%d %d %d)",bg_color.red,bg_color.green,bg_color.blue);
		}
		else if (! bgEffectConfig.isNull() && bgEffectConfig.isArray() && BGCONFIG_ARRAY.size() == 1 && BGCONFIG_ARRAY.at(0).isString())
		{
			const QString bgEffectName = BGCONFIG_ARRAY.at(0).toString();
			int result = effectConfig.contains("background-effect-args")
//			           ? hyperion->setEffect(bgEffectName, effectConfig["background-effect-args"], BG_PRIORITY, fg_duration_ms)
			           ? hyperion->setEffect(bgEffectName, _qconfig["initialEffect"].toObject()["background-effect-args"].toObject(), BG_PRIORITY, DURATION_INFINITY)
			           : hyperion->setEffect(bgEffectName, BG_PRIORITY, DURATION_INFINITY);
			Info(_log,"Inital background effect '%s' %s", bgEffectName.toUtf8().constData(), ((result == 0) ? "started" : "failed"));
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
	connect( Hyperion::getInstance(), SIGNAL(componentStateChanged(hyperion::Components,bool)), _kodiVideoChecker, SLOT(componentStateChanged(hyperion::Components,bool)));
}

void HyperionDaemon::startNetworkServices()
{
	KODIVideoChecker* kodiVideoChecker = KODIVideoChecker::getInstance();

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
	const std::string mDNSDescr = ( generalConfig["name"].toString("").toStdString()
					+ "@" +
					QHostInfo::localHostName().toStdString()
					);

	// zeroconf udp listener 
	if (_udpListener != nullptr) {
		BonjourServiceRegister *bonjourRegister_udp = new BonjourServiceRegister();
		bonjourRegister_udp->registerService(
					BonjourRecord(mDNSDescr.c_str(), "_hyperiond-rgbled._udp", QString()),
					_udpListener->getPort()
					);
		Debug(_log, "UDP LIstener mDNS responder started");
	}

	// zeroconf json
	BonjourServiceRegister *bonjourRegister_json = new BonjourServiceRegister();
	bonjourRegister_json->registerService( 
				BonjourRecord(mDNSDescr.c_str(), "_hyperiond-json._tcp", QString()),
				_jsonServer->getPort()
				);
	Debug(_log, "Json mDNS responder started");

	// zeroconf proto
	BonjourServiceRegister *bonjourRegister_proto = new BonjourServiceRegister();
	bonjourRegister_proto->registerService(
				BonjourRecord(mDNSDescr.c_str(), "_hyperiond-proto._tcp", QString()),
				_protoServer->getPort()
				);
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
				
			QFile amvideo("/dev/amvideo");
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
				else if ( amvideo.exists() )
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
				Info(  _log, "set screen capture device to '%s'", type.toUtf8().constData());
			}
			
			bool grabberCompState = grabberConfig["enable"].toBool(true);
			if (type == "") { Info( _log, "screen capture device disabled"); grabberCompState = false; }
			else if (type == "framebuffer")   createGrabberFramebuffer(grabberConfig);
			else if (type == "dispmanx") createGrabberDispmanx();
			else if (type == "amlogic")  { createGrabberAmlogic(); createGrabberFramebuffer(grabberConfig); }
			else if (type == "osx")      createGrabberOsx(grabberConfig);
			else if (type == "x11")      createGrabberX11(grabberConfig);
			else { Warning( _log, "unknown framegrabber type '%s'", type.toUtf8().constData()); grabberCompState = false; }
			
			_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_GRABBER, grabberCompState);
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
	QObject::connect(_kodiVideoChecker, SIGNAL(videoMode(VideoMode)), _dispmanx, SLOT(setVideoMode(VideoMode)));
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
	_amlGrabber = new AmlogicWrapper(_grabber_width, _grabber_height, _grabber_frequency, _grabber_priority-1);

	QObject::connect(_kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _amlGrabber, SLOT(setGrabbingMode(GrabbingMode)));
	QObject::connect(_kodiVideoChecker, SIGNAL(videoMode(VideoMode)),       _amlGrabber, SLOT(setVideoMode(VideoMode)));
	QObject::connect(_amlGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
	QObject::connect(_amlGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _hyperion, SLOT(setImage(int, const Image<ColorRgb>&, const int)) );

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

	QObject::connect(_kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _x11Grabber, SLOT(setGrabbingMode(GrabbingMode)));
	QObject::connect(_kodiVideoChecker, SIGNAL(videoMode(VideoMode)),       _x11Grabber, SLOT(setVideoMode(VideoMode)));
	QObject::connect(_x11Grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
	QObject::connect(_x11Grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _hyperion, SLOT(setImage(int, const Image<ColorRgb>&, const int)) );

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
				grabberConfig["device"].toString("/dev/fb0").toStdString(),
				_grabber_width, _grabber_height, _grabber_frequency, _grabber_priority);
	
	QObject::connect(_kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _fbGrabber, SLOT(setGrabbingMode(GrabbingMode)));
	QObject::connect(_kodiVideoChecker, SIGNAL(videoMode(VideoMode)), _fbGrabber, SLOT(setVideoMode(VideoMode)));
	QObject::connect(_fbGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
	QObject::connect(_fbGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _hyperion, SLOT(setImage(int, const Image<ColorRgb>&, const int)) );

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
	
	QObject::connect(_kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _osxGrabber, SLOT(setGrabbingMode(GrabbingMode)));
	QObject::connect(_kodiVideoChecker, SIGNAL(videoMode(VideoMode)), _osxGrabber, SLOT(setVideoMode(VideoMode)));
	QObject::connect(_osxGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
	QObject::connect(_osxGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _hyperion, SLOT(setImage(int, const Image<ColorRgb>&, const int)) );

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
				grabberConfig["device"].toString("auto").toStdString(),
				grabberConfig["input"].toInt(0),
				parseVideoStandard(grabberConfig["standard"].toString("no-change").toStdString()),
				parsePixelFormat(grabberConfig["pixelFormat"].toString("no-change").toStdString()),
				grabberConfig["width"].toInt(-1),
				grabberConfig["height"].toInt(-1),
				grabberConfig["frameDecimation"].toInt(2),
				grabberConfig["sizeDecimation"].toInt(8),
				grabberConfig["redSignalThreshold"].toDouble(0.0),
				grabberConfig["greenSignalThreshold"].toDouble(0.0),
				grabberConfig["blueSignalThreshold"].toDouble(0.0),
				grabberConfig["priority"].toInt(890));
			grabber->set3D(parse3DMode(grabberConfig["mode"].toString("2D").toStdString()));
			grabber->setCropping(
				grabberConfig["cropLeft"].toInt(0),
				grabberConfig["cropRight"].toInt(0),
				grabberConfig["cropTop"].toInt(0),
				grabberConfig["cropBottom"].toInt(0));
			grabber->setSignalDetectionOffset(
				grabberConfig["signalDetectionHorizontalOffsetMin"].toDouble(0.25),
				grabberConfig["signalDetectionVerticalOffsetMin"].toDouble(0.25),
				grabberConfig["signalDetectionHorizontalOffsetMax"].toDouble(0.75),
				grabberConfig["signalDetectionVerticalOffsetMax"].toDouble(0.75));
			Debug(_log, "V4L2 grabber created");

			QObject::connect(grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)));
			QObject::connect(grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _hyperion, SLOT(setImage(int, const Image<ColorRgb>&, const int)));
			if (grabberConfig["useKodiChecker"].toBool(false))
			{
				QObject::connect(_kodiVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), grabber, SLOT(setGrabbingMode(GrabbingMode)));
			}
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
