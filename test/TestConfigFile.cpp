
// STL includes
#include <cstdlib>

// QT includes
#include <QResource>

// JsonSchema includes
#include <utils/jsonschema/QJsonFactory.h>

// hyperion includes
#include <hyperion/LedString.h>
#include "HyperionConfig.h"

bool loadConfig(const QString & configFile)
{
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	////////////////////////////////////////////////////////////
	// read and set the json schema from the resource
	////////////////////////////////////////////////////////////

	QJsonObject schemaJson;
	
	try
	{
		schemaJson = QJsonFactory::readSchema(":/hyperion-schema");
	}
	catch(const std::runtime_error& error)
	{
		throw std::runtime_error(error.what());
	}
	
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);
	
	////////////////////////////////////////////////////////////
	// read and validate the configuration file from the command line
	////////////////////////////////////////////////////////////
	
	const QJsonObject jsonConfig = QJsonFactory::readConfig(configFile);
	
	if (!schemaChecker.validate(jsonConfig))
	{
		QStringList schemaErrors = schemaChecker.getMessages();
		foreach (auto & schemaError, schemaErrors)
		{
			std::cout << "config write validation: " << schemaError.toStdString() << std::endl;
		}
		
		std::cout << "FAILED" << std::endl;
		exit(1);
		return false;
	}

	return true;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Missing required configuration file to test" << std::endl;
		std::cerr << "Usage: test_configfile [configfile]" << std::endl;
		return 0;
	}

	const QString configFile(argv[1]);
	std::cout << "Configuration file selected: " << configFile.toStdString() << std::endl;
	std::cout << "Attemp to load..." << std::endl;
	try
	{
		if (loadConfig(configFile))
			std::cout << "PASSED" << std::endl;
		return 0;
	}
	catch (std::runtime_error exception)
	{
		std::cout << "FAILED" << std::endl;
		std::cout << exception.what() << std::endl;
	}

	return 1;
}
