
// STL includes
#include <cstdlib>

// QT includes
#include <QResource>

// JsonSchema includes
#include <utils/jsonschema/JsonFactory.h>

// hyperion includes
#include <hyperion/LedString.h>

Json::Value loadConfig(const std::string & configFile)
{
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	// read the json schema from the resource
	QResource schemaData(":/hyperion-schema");
	if (!schemaData.isValid()) \
	{
		throw std::runtime_error("Schema not found");
	}

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
	if (argc != 2)
	{
		std::cerr << "Missing required configuration file to test" << std::endl;
		std::cerr << "Usage: test_configfile [configfile]" << std::endl;
		return 0;
	}

	const std::string configFile(argv[1]);
	std::cout << "Configuration file selected: " << configFile.c_str() << std::endl;
	std::cout << "Attemp to load...\t";
	try
	{
		Json::Value value = loadConfig(configFile);
		(void)value;
		std::cout << "PASSED" << std::endl;
	}
	catch (std::runtime_error exception)
	{
		std::cout << "FAILED" << std::endl;
		std::cout << exception.what() << std::endl;
	}

	return 0;
}
