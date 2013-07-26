
// STL includes
#include <cstdlib>

// JsonSchema includes
#include <utils/jsonschema/JsonFactory.h>

// hyperion includes
#include <hyperion/LedString.h>

int main()
{
	std::string homeDir = getenv("RASPILIGHT_HOME");

	const std::string schemaFile = homeDir + "/hyperion.schema.json";
	const std::string configFile = homeDir + "/hyperion.config.json";

	Json::Value config;
	if (JsonFactory::load(schemaFile, configFile, config) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << std::endl;
		return -1;
	}

	const Json::Value& deviceConfig = config["device"];
	const Json::Value& ledConfig    = config["leds"];
	const Json::Value& colorConfig  = config["color"];

	std::cout << "COLOR CONFIGURATION: " << colorConfig.toStyledString() << std::endl;

	const Json::Value& redConfig = colorConfig["red"];
	double redGamma = redConfig["gamma"].asDouble();
	std::cout << "RED GAMMA = " << redGamma << std::endl;
	std::cout << "RED GAMMA = " << colorConfig["red.gamma"].asDouble() << std::endl;
	LedString ledString = LedString::construct(ledConfig, colorConfig);

	return 0;
}
