
// QT includes
#include <QCoreApplication>

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/DispmanxWrapper.h>
#include <hyperion/Hyperion.h>

int main(int argc, char** argv)
{
	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);
	std::cout << "QCoreApplication initialised" << std::endl;

	// Select config and schema file
	const std::string homeDir = getenv("RASPILIGHT_HOME");
	const std::string schemaFile = homeDir + "/hyperion.schema.json";
	const std::string configFile = homeDir + "/hyperion.config.json";

	// Load configuration and check against the schema at the same time
	Json::Value config;
	if (JsonFactory::load(schemaFile, configFile, config) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << std::endl;
		return -1;
	}
	std::cout << "Configuration loaded from: " << configFile << std::endl;

	Hyperion hyperion(config);
	std::cout << "Hyperion created and initialised" << std::endl;

	DispmanxWrapper dispmanx(64, 64, 10, &hyperion);
	dispmanx.start();
	std::cout << "Frame grabber created and started" << std::endl;

	app.exec();
	std::cout << "Application closed" << std::endl;
}
