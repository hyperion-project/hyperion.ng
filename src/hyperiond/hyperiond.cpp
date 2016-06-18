#include <unistd.h>

#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QHostInfo>

#include "HyperionConfig.h"

#include <utils/jsonschema/JsonFactory.h>

#include <hyperion/Hyperion.h>
#include <effectengine/EffectEngine.h>
#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>

#include "hyperiond.h"


HyperionDaemon::HyperionDaemon(std::string configFile, QObject *parent)
	: QObject(parent)
	, _log(Logger::getInstance("MAIN"))
	, _xbmcVideoChecker(nullptr)
	, _jsonServer(nullptr)
	, _protoServer(nullptr)
	, _boblightServer(nullptr)
	, _v4l2Grabber(nullptr)
	, _dispmanx(nullptr)
	, _amlGrabber(nullptr)
	, _fbGrabber(nullptr)
	, _osxGrabber(nullptr)
	, _webConfig(nullptr)
{
	loadConfig(configFile);
	Hyperion::initInstance(_config, configFile);
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
	delete _webConfig;

}

void HyperionDaemon::run()
{
	startBootsequence();
	createXBMCVideoChecker();

	// ---- network services -----
	startNetworkServices();
	_webConfig = new WebConfig(this);

	// ---- grabber -----
	createGrabberV4L2();
	createGrabberDispmanx();
	createGrabberAmlogic();
	createGrabberFramebuffer();
	createGrabberDispmanx();

	#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB)
		ErrorIf(_config.isMember("framegrabber"), log, "No grabber can be instantiated, because all grabbers have been left out from the build");
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


void HyperionDaemon::startBootsequence()
{
	Hyperion *hyperion = Hyperion::getInstance();

	// create boot sequence if the configuration is present
	if (_config.isMember("bootsequence"))
	{
		const Json::Value effectConfig = _config["bootsequence"];

		// Get the parameters for the bootsequence
		const std::string effectName = effectConfig["effect"].asString();
		const unsigned duration_ms   = effectConfig["duration_ms"].asUInt();
		const int priority           = (duration_ms != 0) ? 0 : effectConfig.get("priority",990).asInt();
		const int bootcolor_priority = (priority > 990) ? priority+1 : 990;

		// clear the leds
		ColorRgb boot_color = ColorRgb::BLACK;
		hyperion->setColor(bootcolor_priority, boot_color, 0, false);

		// start boot effect
		if ( ! effectName.empty() )
		{
			int result;
			std::cout << "INFO: Boot sequence '" << effectName << "' ";
			if (effectConfig.isMember("args"))
			{
				std::cout << " (with user defined arguments) ";
				const Json::Value effectConfigArgs = effectConfig["args"];
				result = hyperion->setEffect(effectName, effectConfigArgs, priority, duration_ms);
			}
			else
			{
				result = hyperion->setEffect(effectName, priority, duration_ms);
			}
			std::cout << ((result == 0) ? "started" : "failed") << std::endl;
		}

		// static color
		if ( ! effectConfig["color"].isNull() && effectConfig["color"].isArray() && effectConfig["color"].size() == 3 )
		{
			boot_color = {
				(uint8_t)effectConfig["color"][0].asUInt(),
				(uint8_t)effectConfig["color"][1].asUInt(),
				(uint8_t)effectConfig["color"][2].asUInt()
			};
		}

		hyperion->setColor(bootcolor_priority, boot_color, 0, false);
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
	Hyperion *hyperion = Hyperion::getInstance();
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
		_boblightServer = new BoblightServer(hyperion, boblightServerConfig.get("priority",900).asInt(), boblightServerConfig["port"].asUInt());
		Info(_log, "Boblight server created and started on port %d", _boblightServer->getPort());
	}

	// zeroconf
	const Json::Value & deviceConfig = _config["device"];
	const std::string deviceName = deviceConfig.get("name", "").asString();
	const std::string hostname = QHostInfo::localHostName().toStdString();

	// zeroconf json
	std::string mDNSDescr_json = hostname;
	std::string mDNSService_json = "_hyperiond_json._tcp";
	if (_config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = _config["jsonServer"];
		mDNSDescr_json = jsonServerConfig.get("mDNSDescr", mDNSDescr_json).asString();
		mDNSService_json = jsonServerConfig.get("mDNSService", mDNSService_json).asString();
	}
	
	BonjourServiceRegister *bonjourRegister_json = new BonjourServiceRegister();
	bonjourRegister_json->registerService(BonjourRecord((deviceName + " @ " + mDNSDescr_json).c_str(), mDNSService_json.c_str(),
	                                      QString()), _jsonServer->getPort() );
	Info(_log, "Json mDNS responder started");

	// zeroconf proto
	std::string mDNSDescr_proto = hostname;
	std::string mDNSService_proto = "_hyperiond_proto._tcp";
	if (_config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = _config["protoServer"];
		mDNSDescr_proto = protoServerConfig.get("mDNSDescr", mDNSDescr_proto).asString();
		mDNSService_proto = protoServerConfig.get("mDNSService", mDNSService_proto).asString();
	}
	
	BonjourServiceRegister *bonjourRegister_proto = new BonjourServiceRegister();
	bonjourRegister_proto->registerService(BonjourRecord((deviceName + " @ " + mDNSDescr_proto).c_str(), mDNSService_proto.c_str(),
	                                       QString()), _protoServer->getPort() );
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
