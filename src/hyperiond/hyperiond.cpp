#include <unistd.h>

#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>
#include <QHostInfo>

#include "HyperionConfig.h"

#include <utils/jsonschema/JsonFactory.h>
#include <utils/Logger.h>

#include <hyperion/Hyperion.h>
#include <effectengine/EffectEngine.h>
#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>

#include "hyperiond.h"


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
		Info(Logger::getInstance("MAIN"), "Kodi checker created and started");
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
	Info(Logger::getInstance("MAIN"), "Json server created and started on port %d", jsonServer->getPort());

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
	Info(Logger::getInstance("MAIN"), "Proto server created and started on port %d", protoServer->getPort());

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
	Info(Logger::getInstance("MAIN"), "Json mDNS responder started");

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
	Info(Logger::getInstance("MAIN"), "Proto mDNS responder started");

	// Create Boblight server if configuration is present
	if (config.isMember("boblightServer"))
	{
		const Json::Value & boblightServerConfig = config["boblightServer"];
		boblightServer = new BoblightServer(hyperion, boblightServerConfig.get("priority",900).asInt(), boblightServerConfig["port"].asUInt());
		Info(Logger::getInstance("MAIN"), "Boblight server created and started on port %d", boblightServer->getPort());
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
		Info(Logger::getInstance("MAIN"), "Frame grabber created and started");
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
		Info(Logger::getInstance("MAIN"), "V4L2 grabber created and started");
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
		Info(Logger::getInstance("MAIN"), "AMLOGIC grabber created and started");
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
		Info(Logger::getInstance("MAIN"), "Framebuffer grabber created and started");
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
		Info(Logger::getInstance("MAIN"), "OSX grabber created and started");
	}
#endif
	return osxGrabber;
}
