// C++ includes
#include <cassert>
#include <csignal>
#include <vector>
#include <unistd.h>

// QT includes
#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

// Effect engine includes
#include <effectengine/EffectEngine.h>

#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>
#include <QHostInfo>

// network servers
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>

#include <sys/prctl.h> 

#include "hyperiond.h"


void signal_handler(const int signum)
{
	QCoreApplication::quit();

	// reset signal handler to default (in case this handler is not capable of stopping)
	signal(signum, SIG_DFL);
}


Json::Value loadConfig(const std::string & configFile)
{
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

	const Json::Value jsonConfig = JsonFactory::readJson(configFile);
	schemaChecker.validate(jsonConfig);

	return jsonConfig;
}


void startNewHyperion(int parentPid, std::string hyperionFile, std::string configFile)
{
	if ( fork() == 0 )
	{
		sleep(3);
		execl(hyperionFile.c_str(), hyperionFile.c_str(), "--parent", QString::number(parentPid).toStdString().c_str(), configFile.c_str(), NULL);
		exit(0);
	}
}


void startBootsequence()
{
	Hyperion *hyperion = Hyperion::getInstance();
	const Json::Value &config = hyperion->getJsonConfig();

	// create boot sequence if the configuration is present
	if (config.isMember("bootsequence"))
	{
		const Json::Value effectConfig = config["bootsequence"];

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


// create XBMC video checker if the configuration is present
XBMCVideoChecker* createXBMCVideoChecker()
{
	XBMCVideoChecker* xbmcVideoChecker = nullptr;
	const Json::Value &config = Hyperion::getInstance()->getJsonConfig();
	if (config.isMember("xbmcVideoChecker"))
	{
		const Json::Value & videoCheckerConfig = config["xbmcVideoChecker"];
		xbmcVideoChecker = XBMCVideoChecker::initInstance(
			videoCheckerConfig["xbmcAddress"].asString(),
			videoCheckerConfig["xbmcTcpPort"].asUInt(),
			videoCheckerConfig["grabVideo"].asBool(),
			videoCheckerConfig["grabPictures"].asBool(),
			videoCheckerConfig["grabAudio"].asBool(),
			videoCheckerConfig["grabMenu"].asBool(),
			videoCheckerConfig.get("grabPause", true).asBool(),
			videoCheckerConfig.get("grabScreensaver", true).asBool(),
			videoCheckerConfig.get("enable3DDetection", true).asBool());

		xbmcVideoChecker->start();
		std::cout << "INFO: Kodi checker created and started" << std::endl;
	}
	return xbmcVideoChecker;
}

void startNetworkServices(JsonServer* &jsonServer, ProtoServer* &protoServer, BoblightServer* &boblightServer)
{
	Hyperion *hyperion = Hyperion::getInstance();
	XBMCVideoChecker* xbmcVideoChecker = XBMCVideoChecker::getInstance();
	const Json::Value &config = hyperion->getJsonConfig();

	// Create Json server if configuration is present
	unsigned int jsonPort = 19444;
	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = config["jsonServer"];
		//jsonEnable = jsonServerConfig.get("enable", true).asBool();
		jsonPort   = jsonServerConfig.get("port", jsonPort).asUInt();
	}

	jsonServer = new JsonServer(hyperion, jsonPort );
	std::cout << "INFO: Json server created and started on port " << jsonServer->getPort() << std::endl;

	// Create Proto server if configuration is present
	unsigned int protoPort = 19445;
	if (config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = config["protoServer"];
		//protoEnable = protoServerConfig.get("enable", true).asBool();
		protoPort  = protoServerConfig.get("port", protoPort).asUInt();
	}

	protoServer = new ProtoServer(hyperion, protoPort );
	if (xbmcVideoChecker != nullptr)
	{
		QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), protoServer, SIGNAL(grabbingMode(GrabbingMode)));
		QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), protoServer, SIGNAL(videoMode(VideoMode)));
	}
	std::cout << "INFO: Proto server created and started on port " << protoServer->getPort() << std::endl;

	const Json::Value & deviceConfig = config["device"];
	const std::string deviceName = deviceConfig.get("name", "").asString();

	const std::string hostname = QHostInfo::localHostName().toStdString();
	
	std::string mDNSDescr_json = hostname;
	std::string mDNSService_json = "_hyperiond_json._tcp";
	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = config["jsonServer"];
		mDNSDescr_json = jsonServerConfig.get("mDNSDescr", mDNSDescr_json).asString();
		mDNSService_json = jsonServerConfig.get("mDNSService", mDNSService_json).asString();
	}
	
	BonjourServiceRegister *bonjourRegister_json = new BonjourServiceRegister();
	bonjourRegister_json->registerService(BonjourRecord((deviceName + " @ " + mDNSDescr_json).c_str(), mDNSService_json.c_str(),
	                                      QString()), jsonServer->getPort() );
	std::cout << "INFO: Json mDNS responder started" << std::endl;

	std::string mDNSDescr_proto = hostname;
	std::string mDNSService_proto = "_hyperiond_proto._tcp";
	if (config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = config["protoServer"];
		mDNSDescr_proto = protoServerConfig.get("mDNSDescr", mDNSDescr_proto).asString();
		mDNSService_proto = protoServerConfig.get("mDNSService", mDNSService_proto).asString();
	}
	
	BonjourServiceRegister *bonjourRegister_proto = new BonjourServiceRegister();
	bonjourRegister_proto->registerService(BonjourRecord((deviceName + " @ " + mDNSDescr_proto).c_str(), mDNSService_proto.c_str(),
	                                       QString()), protoServer->getPort() );
	std::cout << "INFO: Proto mDNS responder started" << std::endl;

	// Create Boblight server if configuration is present
	if (config.isMember("boblightServer"))
	{
		const Json::Value & boblightServerConfig = config["boblightServer"];
		boblightServer = new BoblightServer(hyperion, boblightServerConfig.get("priority",900).asInt(), boblightServerConfig["port"].asUInt());
		std::cout << "INFO: Boblight server created and started on port " << boblightServer->getPort() << std::endl;
	}
}

DispmanxWrapper* createGrabberDispmanx(ProtoServer* &protoServer)
{
	DispmanxWrapper* dispmanx = nullptr;
#ifdef ENABLE_DISPMANX
	XBMCVideoChecker* xbmcVideoChecker = XBMCVideoChecker::getInstance();
	const Json::Value &config = Hyperion::getInstance()->getJsonConfig();

	// Construct and start the frame-grabber if the configuration is present
	if (config.isMember("framegrabber"))
	{
		const Json::Value & frameGrabberConfig = config["framegrabber"];
		dispmanx = new DispmanxWrapper(
			frameGrabberConfig["width"].asUInt(),
			frameGrabberConfig["height"].asUInt(),
			frameGrabberConfig["frequency_Hz"].asUInt(),
			frameGrabberConfig.get("priority",900).asInt());
		dispmanx->setCropping(
					frameGrabberConfig.get("cropLeft", 0).asInt(),
					frameGrabberConfig.get("cropRight", 0).asInt(),
					frameGrabberConfig.get("cropTop", 0).asInt(),
					frameGrabberConfig.get("cropBottom", 0).asInt());

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), dispmanx, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), dispmanx, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(dispmanx, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		dispmanx->start();
		std::cout << "INFO: Frame grabber created and started" << std::endl;
	}
#endif
	return dispmanx;
}


V4L2Wrapper* createGrabberV4L2(ProtoServer* &protoServer )
{
	// construct and start the v4l2 grabber if the configuration is present
	V4L2Wrapper* v4l2Grabber = nullptr;
#ifdef ENABLE_V4L2
	const Json::Value &config = Hyperion::getInstance()->getJsonConfig();
	if (config.isMember("grabber-v4l2"))
	{
		const Json::Value & grabberConfig = config["grabber-v4l2"];
		v4l2Grabber = new V4L2Wrapper(
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
		v4l2Grabber->set3D(parse3DMode(grabberConfig.get("mode", "2D").asString()));
		v4l2Grabber->setCropping(
					grabberConfig.get("cropLeft", 0).asInt(),
					grabberConfig.get("cropRight", 0).asInt(),
					grabberConfig.get("cropTop", 0).asInt(),
					grabberConfig.get("cropBottom", 0).asInt());

		QObject::connect(v4l2Grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		v4l2Grabber->start();
		std::cout << "INFO: V4L2 grabber created and started" << std::endl;
	}
#endif
	return v4l2Grabber;
}

AmlogicWrapper* createGrabberAmlogic(ProtoServer* &protoServer)
{
	AmlogicWrapper* amlGrabber = nullptr;
#ifdef ENABLE_AMLOGIC
	XBMCVideoChecker* xbmcVideoChecker = XBMCVideoChecker::getInstance();
	const Json::Value &config = Hyperion::getInstance()->getJsonConfig();

	// Construct and start the framebuffer grabber if the configuration is present
	if (config.isMember("amlgrabber"))
	{
		const Json::Value & grabberConfig = config["amlgrabber"];
		amlGrabber = new AmlogicWrapper(
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt());

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), amlGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)),       amlGrabber, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(amlGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		amlGrabber->start();
		std::cout << "INFO: AMLOGIC grabber created and started" << std::endl;
	}
#endif
	return amlGrabber;
}


FramebufferWrapper* createGrabberFramebuffer(ProtoServer* &protoServer)
{
	FramebufferWrapper* fbGrabber = nullptr;
#ifdef ENABLE_FB
	XBMCVideoChecker* xbmcVideoChecker = XBMCVideoChecker::getInstance();
	const Json::Value &config = Hyperion::getInstance()->getJsonConfig();

	// Construct and start the framebuffer grabber if the configuration is present
	if (config.isMember("framebuffergrabber") || config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = config.isMember("framebuffergrabber")? config["framebuffergrabber"] : config["framegrabber"];
		fbGrabber = new FramebufferWrapper(
			grabberConfig.get("device", "/dev/fb0").asString(),
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt());

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), fbGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), fbGrabber, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(fbGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		fbGrabber->start();
		std::cout << "INFO: Framebuffer grabber created and started" << std::endl;
	}
#endif
	return fbGrabber;
}


OsxWrapper* createGrabberOsx(ProtoServer* &protoServer)
{
	OsxWrapper* osxGrabber = nullptr;
#ifdef ENABLE_OSX
	XBMCVideoChecker* xbmcVideoChecker = XBMCVideoChecker::getInstance();
	const Json::Value &config = Hyperion::getInstance()->getJsonConfig();

	// Construct and start the osx grabber if the configuration is present
	if (config.isMember("osxgrabber") || config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = config.isMember("osxgrabber")? config["osxgrabber"] : config["framegrabber"];
		osxGrabber = new OsxWrapper(
									grabberConfig.get("display", 0).asUInt(),
									grabberConfig["width"].asUInt(),
									grabberConfig["height"].asUInt(),
									grabberConfig["frequency_Hz"].asUInt(),
									grabberConfig.get("priority",900).asInt());

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), osxGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), osxGrabber, SLOT(setVideoMode(VideoMode)));
		}
		
		QObject::connect(osxGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		osxGrabber->start();
		std::cout << "INFO: OSX grabber created and started" << std::endl;
	}
#endif
	return osxGrabber;
}
