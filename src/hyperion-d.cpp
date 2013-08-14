
// QT includes
#include <QCoreApplication>

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/DispmanxWrapper.h>
#include <hyperion/Hyperion.h>

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);

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

	Hyperion hyperion(config);

	DispmanxWrapper dispmanx(64, 64, 10, &hyperion);
	dispmanx.start();

	app.exec();
}
