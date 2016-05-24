// C++ includes
#include <cassert>
#include <csignal>
#include <vector>

// QT includes
#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>

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

#ifdef ENABLE_AMLOGIC
#include <grabber/AmlogicWrapper.h>
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
		throw std::runtime_error("ERROR: Json schema wrong: " + jsonReader.getFormattedErrorMessages())	;
	}
	JsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);

	const Json::Value jsonConfig = JsonFactory::readJson(configFile);
	schemaChecker.validate(jsonConfig);

	return jsonConfig;
}

int main(int argc, char** argv)
{
	std::cout
		<< "Hyperion Ambilight Deamon" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION_ID << std::endl
		<< "\tBuild Time: " << __DATE__ << " " << __TIME__ << std::endl;

	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	if (argc < 2)
	{
		std::cout << "ERROR: Missing required configuration file. Usage:" << std::endl;
		std::cout << "hyperiond [config.file]" << std::endl;
		return 1;
	}

	int argvId = 1;
	for ( int i=1; i<argc;i++)
	{
		if ( QFile::exists(argv[i]) )
		{
			argvId = i;
			break;
		}
	}

	const std::string configFile = argv[argvId];
	std::cout << "INFO: Selected configuration file: " << configFile.c_str() << std::endl;
	const Json::Value config = loadConfig(configFile);

	Hyperion hyperion(config);
	std::cout << "INFO: Hyperion started and initialised" << std::endl;

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
		hyperion.setColor(bootcolor_priority, boot_color, 0, false);

		// start boot effect
		if ( ! effectName.empty() )
		{
			int result;
			std::cout << "INFO: Boot sequence '" << effectName << "' ";
			if (effectConfig.isMember("args"))
			{
				std::cout << " (with user defined arguments) ";
				const Json::Value effectConfigArgs = effectConfig["args"];
				result = hyperion.setEffect(effectName, effectConfigArgs, priority, duration_ms);
			}
			else
			{
				result = hyperion.setEffect(effectName, priority, duration_ms);
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

		hyperion.setColor(bootcolor_priority, boot_color, 0, false);
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
		std::cout << "INFO: Kodi checker created and started" << std::endl;
	}

// ---- network services -----

	// Create Json server if configuration is present
	JsonServer * jsonServer = nullptr;
	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = config["jsonServer"];
		jsonServer = new JsonServer(&hyperion, jsonServerConfig["port"].asUInt());
		std::cout << "INFO: Json server created and started on port " << jsonServer->getPort() << std::endl;
	}

#ifdef ENABLE_PROTOBUF
	// Create Proto server if configuration is present
	ProtoServer * protoServer = nullptr;
	if (config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = config["protoServer"];
		protoServer = new ProtoServer(&hyperion, protoServerConfig["port"].asUInt() );
		std::cout << "INFO: Proto server created and started on port " << protoServer->getPort() << std::endl;
	}
#endif

	// Create Boblight server if configuration is present
	BoblightServer * boblightServer = nullptr;
	if (config.isMember("boblightServer"))
	{
		const Json::Value & boblightServerConfig = config["boblightServer"];
		boblightServer = new BoblightServer(&hyperion, boblightServerConfig.get("priority",900).asInt(), boblightServerConfig["port"].asUInt());
		std::cout << "INFO: Boblight server created and started on port " << boblightServer->getPort() << std::endl;
	}

// ---- grabber -----

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
			frameGrabberConfig.get("priority",900).asInt(),
			&hyperion);
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

		#ifdef ENABLE_PROTOBUF
		QObject::connect(dispmanx, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
		#endif

		dispmanx->start();
		std::cout << "INFO: Frame grabber created and started" << std::endl;
	}
#else
#if !defined(ENABLE_OSX) && !defined(ENABLE_FB)
	if (config.isMember("framegrabber"))
	{
		std::cerr << "ERRROR: The dispmanx framegrabber can not be instantiated, because it has been left out from the build" << std::endl;
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
					grabberConfig.get("priority", 900).asInt());
		v4l2Grabber->set3D(parse3DMode(grabberConfig.get("mode", "2D").asString()));
		v4l2Grabber->setCropping(
					grabberConfig.get("cropLeft", 0).asInt(),
					grabberConfig.get("cropRight", 0).asInt(),
					grabberConfig.get("cropTop", 0).asInt(),
					grabberConfig.get("cropBottom", 0).asInt());

		#ifdef ENABLE_PROTOBUF
		QObject::connect(v4l2Grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
		#endif

		v4l2Grabber->start();
		std::cout << "INFO: V4L2 grabber created and started" << std::endl;
	}
#else
	if (config.isMember("grabber-v4l2"))
	{
		std::cerr << "ERROR: The v4l2 grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}

#endif

#ifdef ENABLE_AMLOGIC
	// Construct and start the framebuffer grabber if the configuration is present
	AmlogicWrapper * amlGrabber = nullptr;
	if (config.isMember("amlgrabber"))
	{
		const Json::Value & grabberConfig = config["amlgrabber"];
		amlGrabber = new AmlogicWrapper(
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt(),
			&hyperion);

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), amlGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)),       amlGrabber, SLOT(setVideoMode(VideoMode)));
		}

		#ifdef ENABLE_PROTOBUF
		QObject::connect(amlGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
		#endif

		amlGrabber->start();
		std::cout << "INFO: AMLOGIC grabber created and started" << std::endl;
	}
#else
	if (config.isMember("amlgrabber"))
	{
		std::cerr << "ERROR: The AMLOGIC grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif

#ifdef ENABLE_FB
	// Construct and start the framebuffer grabber if the configuration is present
	FramebufferWrapper * fbGrabber = nullptr;
	if (config.isMember("framebuffergrabber") || config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = config.isMember("framebuffergrabber")? config["framebuffergrabber"] : config["framegrabber"];
		fbGrabber = new FramebufferWrapper(
			grabberConfig.get("device", "/dev/fb0").asString(),
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt(),
			&hyperion);

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), fbGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), fbGrabber, SLOT(setVideoMode(VideoMode)));
		}

		#ifdef ENABLE_PROTOBUF
		QObject::connect(fbGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
		#endif

		fbGrabber->start();
		std::cout << "INFO: Framebuffer grabber created and started" << std::endl;
	}
#else
	if (config.isMember("framebuffergrabber"))
	{
		std::cerr << "ERROR: The framebuffer grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX)
	else if (config.isMember("framegrabber"))
	{
		std::cerr << "ERROR: The framebuffer grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif
#endif

#ifdef ENABLE_OSX
	// Construct and start the osx grabber if the configuration is present
	OsxWrapper * osxGrabber = nullptr;
	if (config.isMember("osxgrabber") || config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = config.isMember("osxgrabber")? config["osxgrabber"] : config["framegrabber"];
		osxGrabber = new OsxWrapper(
									grabberConfig.get("display", 0).asUInt(),
									grabberConfig["width"].asUInt(),
									grabberConfig["height"].asUInt(),
									grabberConfig["frequency_Hz"].asUInt(),
									grabberConfig.get("priority",900).asInt(),
									&hyperion );

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), osxGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), osxGrabber, SLOT(setVideoMode(VideoMode)));
		}
		
		#ifdef ENABLE_PROTOBUF
		QObject::connect(osxGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );
		#endif

		osxGrabber->start();
		std::cout << "INFO: OSX grabber created and started" << std::endl;
	}
#else
	if (config.isMember("osxgrabber"))
	{
		std::cerr << "ERROR: The osx grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_FB)
	else if (config.isMember("framegrabber"))
	{
		std::cerr << "ERROR: The osx grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif
#endif


	// run the application
	int rc = app.exec();
	std::cout << "INFO: Application closed with code " << rc << std::endl;

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
