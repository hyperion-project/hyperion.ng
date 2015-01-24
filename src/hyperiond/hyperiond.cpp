// C++ includes
#include <cassert>
#include <csignal>

// QT includes
#include <QCoreApplication>
#include <QResource>
#include <QLocale>

// config includes
#include "HyperionConfig.h"

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

#ifdef ENABLE_DISPMANX
// Dispmanx grabber includes
#include <grabber/DispmanxWrapper.h>
#endif

#ifdef ENABLE_V4L2
// v4l2 grabber
#include <grabber/V4L2Wrapper.h>
#endif

#ifdef ENABLE_FB
// Framebuffer grabber includes
#include <grabber/FramebufferWrapper.h>
#endif

#ifdef ENABLE_OSX
// OSX grabber includes
#include <grabber/OsxWrapper.h>
#endif

// XBMC Video checker includes
#include <xbmcvideochecker/XBMCVideoChecker.h>

// Effect engine includes
#include <effectengine/EffectEngine.h>

// JsonServer includes
#include <jsonserver/JsonServer.h>

#ifdef ENABLE_PROTOBUF
// ProtoServer includes
#include <protoserver/ProtoServer.h>
#endif

// BoblightServer includes
#include <boblightserver/BoblightServer.h>

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
		throw std::runtime_error("Schema error: " + jsonReader.getFormattedErrorMessages())	;
	}
	JsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);

	const Json::Value jsonConfig = JsonFactory::readJson(configFile);
	schemaChecker.validate(jsonConfig);

	return jsonConfig;
}

int main(int argc, char** argv)
{
	std::cout << "Application build time: " << __DATE__ << " " << __TIME__ << std::endl;

	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);
	std::cout << "QCoreApplication initialised" << std::endl;

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	if (argc < 2)
	{
		std::cout << "Missing required configuration file. Usage:" << std::endl;
		std::cout << "hyperiond [config.file]" << std::endl;
		return 1;
	}

	const std::string configFile = argv[1];
	std::cout << "Selected configuration file: " << configFile.c_str() << std::endl;
	const Json::Value config = loadConfig(configFile);

	Hyperion hyperion(config);
	std::cout << "Hyperion created and initialised" << std::endl;

	// create boot sequence if the configuration is present
	if (config.isMember("bootsequence"))
	{
		const Json::Value effectConfig = config["bootsequence"];

		// Get the parameters for the bootsequence
		const std::string effectName = effectConfig["effect"].asString();
		const unsigned duration_ms   = effectConfig["duration_ms"].asUInt();
		const int priority = 0;
		
		hyperion.setColor(priority+1, ColorRgb::BLACK, duration_ms, false);

		if (effectConfig.isMember("args"))
		{
			const Json::Value effectConfigArgs = effectConfig["args"];
			if (hyperion.setEffect(effectName, effectConfigArgs, priority, duration_ms) == 0)
			{
					std::cout << "Boot sequence(" << effectName << ") with user-defined arguments created and started" << std::endl;
			}
			else
			{
					std::cout << "Failed to start boot sequence: " << effectName << " with user-defined arguments" << std::endl;
			}
		}
		else
		{
			if (hyperion.setEffect(effectName, priority, duration_ms) == 0)
			{
				std::cout << "Boot sequence(" << effectName << ") created and started" << std::endl;
			}
			else
			{
				std::cout << "Failed to start boot sequence: " << effectName << std::endl;
			}
		}
	}

	// create XBMC video checker if the configuration is present
	XBMCVideoChecker * xbmcVideoChecker = nullptr;
	if (config.isMember("xbmcVideoChecker"))
	{
		const Json::Value & videoCheckerConfig = config["xbmcVideoChecker"];
		xbmcVideoChecker = new XBMCVideoChecker(
			videoCheckerConfig["xbmcAddress"].asString(),
			videoCheckerConfig["xbmcTcpPort"].asUInt(),
			videoCheckerConfig["grabVideo"].asBool(),
			videoCheckerConfig["grabPictures"].asBool(),
			videoCheckerConfig["grabAudio"].asBool(),
			videoCheckerConfig["grabMenu"].asBool(),
			videoCheckerConfig.get("grabScreensaver", true).asBool(),
			videoCheckerConfig.get("enable3DDetection", true).asBool());

		xbmcVideoChecker->start();
		std::cout << "XBMC video checker created and started" << std::endl;
	}

#ifdef ENABLE_DISPMANX
	// Construct and start the frame-grabber if the configuration is present
	DispmanxWrapper * dispmanx = nullptr;
	if (config.isMember("framegrabber"))
	{
		const Json::Value & frameGrabberConfig = config["framegrabber"];
		dispmanx = new DispmanxWrapper(
			frameGrabberConfig["width"].asUInt(),
			frameGrabberConfig["height"].asUInt(),
			frameGrabberConfig["frequency_Hz"].asUInt(),
			&hyperion);

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), dispmanx, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), dispmanx, SLOT(setVideoMode(VideoMode)));
		}

		dispmanx->start();
		std::cout << "Frame grabber created and started" << std::endl;
	}
#else
#if !defined(ENABLE_OSX) && !defined(ENABLE_FB)
	if (config.isMember("framegrabber"))
	{
		std::cerr << "The dispmanx framegrabber can not be instantiated, becuse it has been left out from the build" << std::endl;
	}
#endif
#endif

#ifdef ENABLE_V4L2
	// construct and start the v4l2 grabber if the configuration is present
	V4L2Wrapper * v4l2Grabber = nullptr;
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
					&hyperion,
					grabberConfig.get("priority", 800).asInt());
		v4l2Grabber->set3D(parse3DMode(grabberConfig.get("mode", "2D").asString()));
		v4l2Grabber->setCropping(
					grabberConfig.get("cropLeft", 0).asInt(),
					grabberConfig.get("cropRight", 0).asInt(),
					grabberConfig.get("cropTop", 0).asInt(),
					grabberConfig.get("cropBottom", 0).asInt());

		v4l2Grabber->start();
		std::cout << "V4l2 grabber created and started" << std::endl;
	}
#else
	if (config.isMember("grabber-v4l2"))
	{
		std::cerr << "The v4l2 grabber can not be instantiated, becuse it has been left out from the build" << std::endl;
	}
#endif
	
#ifdef ENABLE_FB
	// Construct and start the framebuffer grabber if the configuration is present
	FramebufferWrapper * fbGrabber = nullptr;
	if (config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = config["framegrabber"];
		fbGrabber = new FramebufferWrapper(
			grabberConfig.get("device", "/dev/fb0").asString(),
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			&hyperion);

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), fbGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), fbGrabber, SLOT(setVideoMode(VideoMode)));
		}

		fbGrabber->start();
		std::cout << "Framebuffer grabber created and started" << std::endl;
	}
#else
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX)
	if (config.isMember("framegrabber"))
	{
		std::cerr << "The framebuffer grabber can not be instantiated, becuse it has been left out from the build" << std::endl;
	}
#endif
#endif
    
#ifdef ENABLE_OSX
    // Construct and start the osx grabber if the configuration is present
    OsxWrapper * osxGrabber = nullptr;
    if (config.isMember("framegrabber"))
    {
        const Json::Value & grabberConfig = config["framegrabber"];
        osxGrabber = new OsxWrapper(
                                           grabberConfig.get("display", 0).asUInt(),
                                           grabberConfig["width"].asUInt(),
                                           grabberConfig["height"].asUInt(),
                                           grabberConfig["frequency_Hz"].asUInt(),
                                           &hyperion);
        
        if (xbmcVideoChecker != nullptr)
        {
            QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), osxGrabber, SLOT(setGrabbingMode(GrabbingMode)));
            QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), osxGrabber, SLOT(setVideoMode(VideoMode)));
        }
        
        osxGrabber->start();
        std::cout << "OSX grabber created and started" << std::endl;
    }
#else
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_FB)
    if (config.isMember("framegrabber"))
    {
        std::cerr << "The osx grabber can not be instantiated, becuse it has been left out from the build" << std::endl;
    }
#endif
#endif

	// Create Json server if configuration is present
	JsonServer * jsonServer = nullptr;
	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = config["jsonServer"];
		jsonServer = new JsonServer(&hyperion, jsonServerConfig["port"].asUInt());
		std::cout << "Json server created and started on port " << jsonServer->getPort() << std::endl;
	}

#ifdef ENABLE_PROTOBUF
	// Create Proto server if configuration is present
	ProtoServer * protoServer = nullptr;
	if (config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = config["protoServer"];
		protoServer = new ProtoServer(&hyperion, protoServerConfig["port"].asUInt());
		std::cout << "Proto server created and started on port " << protoServer->getPort() << std::endl;
	}
#endif

	// Create Boblight server if configuration is present
	BoblightServer * boblightServer = nullptr;
	if (config.isMember("boblightServer"))
	{
		const Json::Value & boblightServerConfig = config["boblightServer"];
		boblightServer = new BoblightServer(&hyperion, boblightServerConfig["port"].asUInt());
		std::cout << "Boblight server created and started on port " << boblightServer->getPort() << std::endl;
	}

	// run the application
	int rc = app.exec();
	std::cout << "Application closed with code " << rc << std::endl;

	// Delete all component
#ifdef ENABLE_DISPMANX
	delete dispmanx;
#endif
#ifdef ENABLE_FB
	delete fbGrabber;
#endif
#ifdef ENABLE_OSX
    delete osxGrabber;
#endif
#ifdef ENABLE_V4L2
	delete v4l2Grabber;
#endif
	delete xbmcVideoChecker;
	delete jsonServer;
#ifdef ENABLE_PROTOBUF
	delete protoServer;
#endif
	delete boblightServer;

	// leave application
	return rc;
}
