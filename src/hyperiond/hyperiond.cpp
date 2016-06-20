#include <unistd.h>

#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QHostInfo>
#include <cstdint>
#include <limits>

#include "HyperionConfig.h"

#include <utils/jsonschema/JsonFactory.h>

#include <hyperion/Hyperion.h>
#include <effectengine/EffectEngine.h>
#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>
#include <udplistener/UDPListener.h>

#include "hyperiond.h"


HyperionDaemon::HyperionDaemon(std::string configFile, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("MAIN"))
	, _xbmcVideoChecker(nullptr)
	, _jsonServer(nullptr)
	, _protoServer(nullptr)
	, _boblightServer(nullptr)
	, _udpListener(nullptr)
	, _v4l2Grabber(nullptr)
	, _dispmanx(nullptr)
	, _amlGrabber(nullptr)
	, _fbGrabber(nullptr)
	, _osxGrabber(nullptr)
	, _webConfig(nullptr)
	, _hyperion(nullptr)
{
	loadConfig(configFile);
	_hyperion = Hyperion::initInstance(_config, configFile);
	Info(_log, "Hyperion started and initialised");
}

HyperionDaemon::~HyperionDaemon()
{
	delete _amlGrabber;
	delete _dispmanx;
	delete _fbGrabber;
	delete _osxGrabber;
	delete _v4l2Grabber;
	delete _xbmcVideoChecker;
	delete _jsonServer;
	delete _protoServer;
	delete _boblightServer;
	delete _udpListener;
	delete _webConfig;
	delete _hyperion;

}

void HyperionDaemon::run()
{
	startInitialEffect();
	createXBMCVideoChecker();

	// ---- network services -----
	startNetworkServices();
	_webConfig = new WebConfig(this);

	// ---- grabber -----
	createGrabberV4L2();
	createGrabberDispmanx();
	createGrabberAmlogic();
	createGrabberFramebuffer();

	#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB)
		ErrorIf(_config.isMember("framegrabber"), _log, "No grabber can be instantiated, because all grabbers have been left out from the build");
	#endif

}

void HyperionDaemon::loadConfig(const std::string & configFile)
{
	Info(_log, "Selected configuration file: %s", configFile.c_str() );
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	// read the json schema from the resource
	QResource schemaData(":/hyperion-schema");
	assert(schemaData.isValid());

	Json::Reader jsonReader;
	Json::Value schemaJson;
	if (!jsonReader.parse(reinterpret_cast<const char *>(schemaData.data()), reinterpret_cast<const char *>(schemaData.data()) + schemaData.size(), schemaJson, false))
	{
		throw std::runtime_error("ERROR: Json schema wrong: " + jsonReader.getFormattedErrorMessages())	;
	}
	JsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);

	_config = JsonFactory::readJson(configFile);
	schemaChecker.validate(_config);
}


void HyperionDaemon::startInitialEffect()
{
	Hyperion *hyperion = Hyperion::getInstance();

	// create boot sequence if the configuration is present
	if (_config.isMember("initialEffect"))
	{
		const Json::Value effectConfig = _config["initialEffect"];
		const int HIGHEST_PRIORITY = 0;
		const int DURATION_INFINITY = 0;
		const int LOWEST_PRIORITY = std::numeric_limits<int>::max()-1;

		// clear the leds
		hyperion->setColor(HIGHEST_PRIORITY, ColorRgb::BLACK, DURATION_INFINITY, false);

		// initial foreground effect/color
		const Json::Value fgEffectConfig = effectConfig["foreground-effect"];
		const int fg_duration_ms = effectConfig.get("foreground-effect-duration_ms",3000).asUInt();
		if ( ! fgEffectConfig.isNull() && fgEffectConfig.isArray() && fgEffectConfig.size() == 3 )
		{
			ColorRgb fg_color = {
				(uint8_t)fgEffectConfig[0].asUInt(),
				(uint8_t)fgEffectConfig[1].asUInt(),
				(uint8_t)fgEffectConfig[2].asUInt()
			};
			hyperion->setColor(HIGHEST_PRIORITY, fg_color, fg_duration_ms, false);
			Info(_log,"Inital foreground color set (%d %d %d)",fg_color.red,fg_color.green,fg_color.blue);
		}
		else if (! fgEffectConfig.isNull() && fgEffectConfig.isString())
		{
			const std::string bgEffectName = fgEffectConfig.asString();
			int result = effectConfig.isMember("foreground-effect-args")
			           ? hyperion->setEffect(bgEffectName, effectConfig["foreground-effect-args"], HIGHEST_PRIORITY, fg_duration_ms)
			           : hyperion->setEffect(bgEffectName, HIGHEST_PRIORITY, fg_duration_ms);
			Info(_log,"Inital foreground effect '%s' %s", bgEffectName.c_str(), ((result == 0) ? "started" : "failed"));
		}

		// initial background effect/color
		const Json::Value bgEffectConfig = effectConfig["background-effect"];
		if ( ! bgEffectConfig.isNull() && bgEffectConfig.isArray() && bgEffectConfig.size() == 3 )
		{
			ColorRgb bg_color = {
				(uint8_t)bgEffectConfig[0].asUInt(),
				(uint8_t)bgEffectConfig[1].asUInt(),
				(uint8_t)bgEffectConfig[2].asUInt()
			};
			hyperion->setColor(LOWEST_PRIORITY, bg_color, DURATION_INFINITY, false);
			Info(_log,"Inital background color set (%d %d %d)",bg_color.red,bg_color.green,bg_color.blue);
		}
		else if (! bgEffectConfig.isNull() && bgEffectConfig.isString())
		{
			const std::string bgEffectName = bgEffectConfig.asString();
			int result = effectConfig.isMember("background-effect-args")
			           ? hyperion->setEffect(bgEffectName, effectConfig["background-effect-args"], LOWEST_PRIORITY, DURATION_INFINITY)
			           : hyperion->setEffect(bgEffectName, LOWEST_PRIORITY, DURATION_INFINITY);
			Info(_log,"Inital background effect '%s' %s", bgEffectName.c_str(), ((result == 0) ? "started" : "failed"));
		}
	}
}


// create XBMC video checker if the _configuration is present
void HyperionDaemon::createXBMCVideoChecker()
{
	if (_config.isMember("xbmcVideoChecker"))
	{
		const Json::Value & videoCheckerConfig = _config["xbmcVideoChecker"];
		_xbmcVideoChecker = XBMCVideoChecker::initInstance(
			videoCheckerConfig["xbmcAddress"].asString(),
			videoCheckerConfig["xbmcTcpPort"].asUInt(),
			videoCheckerConfig["grabVideo"].asBool(),
			videoCheckerConfig["grabPictures"].asBool(),
			videoCheckerConfig["grabAudio"].asBool(),
			videoCheckerConfig["grabMenu"].asBool(),
			videoCheckerConfig.get("grabPause", true).asBool(),
			videoCheckerConfig.get("grabScreensaver", true).asBool(),
			videoCheckerConfig.get("enable3DDetection", true).asBool());

		_xbmcVideoChecker->start();
		Info(_log, "Kodi checker created and started");
	}
}

void HyperionDaemon::startNetworkServices()
{
	XBMCVideoChecker* xbmcVideoChecker = XBMCVideoChecker::getInstance();

	// Create Json server if configuration is present
	unsigned int jsonPort = 19444;
	if (_config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = _config["jsonServer"];
		//jsonEnable = jsonServerConfig.get("enable", true).asBool();
		jsonPort   = jsonServerConfig.get("port", jsonPort).asUInt();
	}

	_jsonServer = new JsonServer(jsonPort );
	Info(_log, "Json server created and started on port %d", _jsonServer->getPort());

	// Create Proto server if configuration is present
	unsigned int protoPort = 19445;
	if (_config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = _config["protoServer"];
		//protoEnable = protoServerConfig.get("enable", true).asBool();
		protoPort  = protoServerConfig.get("port", protoPort).asUInt();
	}

	_protoServer = new ProtoServer(protoPort );
	if (xbmcVideoChecker != nullptr)
	{
		QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _protoServer, SIGNAL(grabbingMode(GrabbingMode)));
		QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), _protoServer, SIGNAL(videoMode(VideoMode)));
	}
	Info(_log, "Proto server created and started on port %d", _protoServer->getPort());

	// Create Boblight server if configuration is present
	if (_config.isMember("boblightServer"))
	{
		const Json::Value & boblightServerConfig = _config["boblightServer"];
		if ( boblightServerConfig.get("enable", true).asBool() )
		{
			_boblightServer = new BoblightServer(
						boblightServerConfig.get("priority",900).asInt(),
						boblightServerConfig["port"].asUInt()
						);
			Info(_log, "Boblight server created and started on port %d", _boblightServer->getPort());
		}
	}

	// Create UDP listener if configuration is present
	if (_config.isMember("udpListener"))
	{
		const Json::Value & udpListenerConfig = _config["udpListener"];
		if ( udpListenerConfig.get("enable", true).asBool() )
		{
			_udpListener = new UDPListener(
						udpListenerConfig.get("priority",700).asInt(),
						udpListenerConfig.get("timeout",10000).asInt(),
						udpListenerConfig.get("port", 2801).asUInt()
					);
			Info(_log, "UDP listener created and started on port %d", _udpListener->getPort());
		}
	}

	// zeroconf description - $leddevicename@$hostname
	const Json::Value & deviceConfig = _config["device"];
	const std::string mDNSDescr = ( deviceConfig.get("name", "").asString()
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
		Info(_log, "UDP LIstener mDNS responder started");
	}

	// zeroconf json
	BonjourServiceRegister *bonjourRegister_json = new BonjourServiceRegister();
	bonjourRegister_json->registerService( 
				BonjourRecord(mDNSDescr.c_str(), "_hyperiond-json._tcp", QString()),
				_jsonServer->getPort()
				);
	Info(_log, "Json mDNS responder started");

	// zeroconf proto
	BonjourServiceRegister *bonjourRegister_proto = new BonjourServiceRegister();
	bonjourRegister_proto->registerService(
				BonjourRecord(mDNSDescr.c_str(), "_hyperiond-proto._tcp", QString()),
				_protoServer->getPort()
				);
	Info(_log, "Proto mDNS responder started");
}

void HyperionDaemon::createGrabberDispmanx()
{
#ifdef ENABLE_DISPMANX
	// Construct and start the frame-grabber if the configuration is present
	if (_config.isMember("framegrabber"))
	{
		const Json::Value & frameGrabberConfig = _config["framegrabber"];
		_dispmanx = new DispmanxWrapper(
			frameGrabberConfig["width"].asUInt(),
			frameGrabberConfig["height"].asUInt(),
			frameGrabberConfig["frequency_Hz"].asUInt(),
			frameGrabberConfig.get("priority",900).asInt());
		_dispmanx->setCropping(
					frameGrabberConfig.get("cropLeft", 0).asInt(),
					frameGrabberConfig.get("cropRight", 0).asInt(),
					frameGrabberConfig.get("cropTop", 0).asInt(),
					frameGrabberConfig.get("cropBottom", 0).asInt());

		if (_xbmcVideoChecker != nullptr)
		{
			QObject::connect(_xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _dispmanx, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(_xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), _dispmanx, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(_dispmanx, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		_dispmanx->start();
		Info(_log, "Frame grabber created and started");
	}
#else
	ErrorIf(_config.isMember("framegrabber"), _log, "The dispmanx framegrabber can not be instantiated, because it has been left out from the build");
#endif
}


void HyperionDaemon::createGrabberV4L2()
{
	// construct and start the v4l2 grabber if the configuration is present
#ifdef ENABLE_V4L2
	if (_config.isMember("grabber-v4l2"))
	{
		const Json::Value & grabberConfig = _config["grabber-v4l2"];
		_v4l2Grabber = new V4L2Wrapper(
					grabberConfig.get("device", "/dev/video0").asString(),
					grabberConfig.get("input", 0).asInt(),
					parseVideoStandard(grabberConfig.get("standard", "no-change").asString()),
					parsePixelFormat(grabberConfig.get("pixelFormat", "no-change").asString()),
					grabberConfig.get("width", -1).asInt(),
					grabberConfig.get("height", -1).asInt(),
					grabberConfig.get("frameDecimation", 2).asInt(),
					grabberConfig.get("sizeDecimation", 8).asInt(),
					grabberConfig.get("redSignalThreshold", 0.0).asDouble(),
					grabberConfig.get("greenSignalThreshold", 0.0).asDouble(),
					grabberConfig.get("blueSignalThreshold", 0.0).asDouble(),
					grabberConfig.get("priority", 900).asInt());
		_v4l2Grabber->set3D(parse3DMode(grabberConfig.get("mode", "2D").asString()));
		_v4l2Grabber->setCropping(
					grabberConfig.get("cropLeft", 0).asInt(),
					grabberConfig.get("cropRight", 0).asInt(),
					grabberConfig.get("cropTop", 0).asInt(),
					grabberConfig.get("cropBottom", 0).asInt());

		QObject::connect(_v4l2Grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		_v4l2Grabber->start();
		Info(_log, "V4L2 grabber created and started");
	}
#else
	ErrorIf(_config.isMember("grabber-v4l2"), _log, "The v4l2 grabber can not be instantiated, because it has been left out from the build");
#endif
}

void HyperionDaemon::createGrabberAmlogic()
{
#ifdef ENABLE_AMLOGIC
	// Construct and start the framebuffer grabber if the configuration is present
	if (_config.isMember("amlgrabber"))
	{
		const Json::Value & grabberConfig = _config["amlgrabber"];
		_amlGrabber = new AmlogicWrapper(
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt());

		if (_xbmcVideoChecker != nullptr)
		{
			QObject::connect(_xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _amlGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(_xbmcVideoChecker, SIGNAL(videoMode(VideoMode)),       _amlGrabber, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(_amlGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		_amlGrabber->start();
		Info(_log, "AMLOGIC grabber created and started");
	}
#else
	ErrorIf(_config.isMember("amlgrabber"), _log, "The AMLOGIC grabber can not be instantiated, because it has been left out from the build");
#endif
}


void HyperionDaemon::createGrabberFramebuffer()
{
#ifdef ENABLE_FB
	// Construct and start the framebuffer grabber if the configuration is present
	if (_config.isMember("framebuffergrabber") || _config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = _config.isMember("framebuffergrabber")? _config["framebuffergrabber"] : _config["framegrabber"];
		_fbGrabber = new FramebufferWrapper(
			grabberConfig.get("device", "/dev/fb0").asString(),
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt());

		if (_xbmcVideoChecker != nullptr)
		{
			QObject::connect(_xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _fbGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(_xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), _fbGrabber, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(_fbGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		_fbGrabber->start();
		Info(_log, "Framebuffer grabber created and started");
	}
#else
	ErrorIf(_config.isMember("framebuffergrabber"), _log, "The framebuffer grabber can not be instantiated, because it has been left out from the build");
#endif
}


void HyperionDaemon::createGrabberOsx()
{
#ifdef ENABLE_OSX
	// Construct and start the osx grabber if the configuration is present
	if (_config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = _config.isMember("osxgrabber")? _config["osxgrabber"] : _config["framegrabber"];
		_osxGrabber = new OsxWrapper(
									grabberConfig.get("display", 0).asUInt(),
									grabberConfig["width"].asUInt(),
									grabberConfig["height"].asUInt(),
									grabberConfig["frequency_Hz"].asUInt(),
									grabberConfig.get("priority",900).asInt());

		if (_xbmcVideoChecker != nullptr)
		{
			QObject::connect(_xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), _osxGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(_xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), _osxGrabber, SLOT(setVideoMode(VideoMode)));
		}
		
		QObject::connect(_osxGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		_osxGrabber->start();
		Info(_log, "OSX grabber created and started");
	}
#else
	ErrorIf(_config.isMember("osxgrabber"), _log, "The osx grabber can not be instantiated, because it has been left out from the build");
#endif
}
