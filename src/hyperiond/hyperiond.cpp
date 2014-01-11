// C++ includes
#include <cassert>
#include <csignal>

// QT includes
#include <QCoreApplication>
#include <QResource>

// config includes
#include "HyperionConfig.h"

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

#ifdef ENABLE_DISPMANX
// Dispmanx grabber includes
#include <dispmanx-grabber/DispmanxWrapper.h>
#endif

// XBMC Video checker includes
#include <xbmcvideochecker/XBMCVideoChecker.h>

// Effect engine includes
#include <effectengine/EffectEngine.h>

// JsonServer includes
#include <jsonserver/JsonServer.h>

// ProtoServer includes
#include <protoserver/ProtoServer.h>

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

		if (hyperion.setEffect(effectName, priority, duration_ms) == 0)
		{
			std::cout << "Boot sequence(" << effectName << ") created and started" << std::endl;
		}
		else
		{
			std::cout << "Failed to start boot sequence: " << effectName << std::endl;
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
	if (config.isMember("framegrabber"))
	{
		std::cerr << "The dispmanx framegrabber can not be instantiated, becuse it has been left out from the build" << std::endl;
	}
#endif

	// Create Json server if configuration is present
	JsonServer * jsonServer = nullptr;
	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = config["jsonServer"];
		jsonServer = new JsonServer(&hyperion, jsonServerConfig["port"].asUInt());
		std::cout << "Json server created and started on port " << jsonServer->getPort() << std::endl;
	}

	// Create Proto server if configuration is present
	ProtoServer * protoServer = nullptr;
	if (config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = config["protoServer"];
		protoServer = new ProtoServer(&hyperion, protoServerConfig["port"].asUInt());
		std::cout << "Proto server created and started on port " << protoServer->getPort() << std::endl;
	}

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
	delete xbmcVideoChecker;
	delete jsonServer;
	delete protoServer;
	delete boblightServer;

	// leave application
	return rc;
}
