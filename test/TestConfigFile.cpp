
// STL includes
#include <cstdlib>

// QT includes
#include <QResource>
#include <QDebug>

// JsonSchema includes
#include <utils/jsonschema/QJsonFactory.h>

// hyperion includes
#include <hyperion/LedString.h>
#include "HyperionConfig.h"

bool loadConfig(const QString & configFile, bool correct)
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
	
	QJsonObject jsonConfig = QJsonFactory::readConfig(configFile);

	if (!correct)
	{
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
	}
	else
	{
		jsonConfig = schemaChecker.getAutoCorrectedConfig(jsonConfig);
		QJsonFactory::writeJson(configFile, jsonConfig);
	}

	return true;
}

void usage()
{
	qDebug() << "Missing required configuration file to test";
	qDebug() << "Usage: test_configfile <option> [configfile]";
	qDebug() << "<option>:";
	qDebug() << "\t--autocorrection - for json auto correction";
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		usage();
		return 0;
	}

	QString option = argv[1];
	QString configFile;

    if (option == "--autocorrection")
		if (argc > 2)
			configFile = argv[2];
		else
		{
			usage();
			return 0;
		}
	else
		configFile = argv[1];

	std::cout << "Configuration file selected: " << configFile.toStdString() << std::endl;
	std::cout << "Attemp to load..." << std::endl;
	try
	{
		if (loadConfig(configFile, (option == "--autocorrection")))
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
