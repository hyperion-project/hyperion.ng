
// C++ includes
#include <csignal>

// QT includes
#include <QCoreApplication>
#include <QResource>

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

// Bootsequence includes
#include <bootsequence/BootSequenceFactory.h>

// Dispmanx grabber includes
#include <dispmanx-grabber/DispmanxWrapper.h>

// XBMC Video checker includes
#include <xbmcvideochecker/XBMCVideoChecker.h>

// JsonServer includes
#include <jsonserver/JsonServer.h>

void signal_handler(const int signum)
{
	QCoreApplication::quit();
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
	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);
	std::cout << "QCoreApplication initialised" << std::endl;

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);

	if (argc < 2)
	{
		std::cout << "Missing required configuration file. Usage:" << std::endl;
		std::cout << "hyperiond [config.file]" << std::endl;
		return 0;
	}

	const std::string configFile = argv[1];
	std::cout << "Selected configuration file: " << configFile.c_str() << std::endl;
	const Json::Value config = loadConfig(configFile);

	Hyperion hyperion(config);
	std::cout << "Hyperion created and initialised" << std::endl;

	BootSequence * bootSequence = BootSequenceFactory::createBootSequence(&hyperion, config["bootsequence"]);
	bootSequence->start();

	const Json::Value & videoCheckerConfig = config["xbmcVideoChecker"];
	XBMCVideoChecker xbmcVideoChecker(videoCheckerConfig["xbmcAddress"].asString(), videoCheckerConfig["xbmcTcpPort"].asUInt(), 1000, &hyperion, 999);
	if (videoCheckerConfig["enable"].asBool())
	{
		xbmcVideoChecker.start();
		std::cout << "XBMC video checker created and started" << std::endl;
	}

	// Construct and start the frame-grabber
	const Json::Value & frameGrabberConfig = config["framegrabber"];
	DispmanxWrapper dispmanx(
			frameGrabberConfig["width"].asUInt(),
			frameGrabberConfig["height"].asUInt(),
			frameGrabberConfig["frequency_Hz"].asUInt(),
			&hyperion);
	dispmanx.start();
	std::cout << "Frame grabber created and started" << std::endl;

	JsonServer jsonServer(&hyperion);
	std::cout << "Json server created and started on port " << jsonServer.getPort() << std::endl;

	// run the application
	int rc = app.exec();
	std::cout << "Application closed" << std::endl;

	// Stop the frame grabber
	dispmanx.stop();

	// Delete the boot sequence
	delete bootSequence;

	// Clear all colors (switchting off all leds)
	hyperion.clearall();

	return rc;
}
