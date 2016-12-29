
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
	QJsonParseError error;

	////////////////////////////////////////////////////////////
	// read and set the json schema from the resource
	////////////////////////////////////////////////////////////

	QFile schemaData(":/hyperion-schema-"+QString::number(CURRENT_CONFIG_VERSION));

	if (!schemaData.open(QIODevice::ReadOnly))
	{
		std::stringstream error;
		error << "Schema not found: " << schemaData.errorString().toStdString();
		throw std::runtime_error(error.str());
	}

	QByteArray schema = schemaData.readAll();
	QJsonDocument schemaJson = QJsonDocument::fromJson(schema, &error);

	if (error.error != QJsonParseError::NoError)
	{
		// report to the user the failure and their locations in the document.
		int errorLine(0), errorColumn(0);
		
		for( int i=0, count=qMin( error.offset,schema.size()); i<count; ++i )
		{
			++errorColumn;
			if(schema.at(i) == '\n' )
			{
				errorColumn = 0;
				++errorLine;
			}
		}
		
		std::stringstream sstream;
		sstream << "Schema error: " << error.errorString().toStdString() << " at Line: " << errorLine << ", Column: " << errorColumn;

		throw std::runtime_error(sstream.str());
	}
	
	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson.object());
	
	////////////////////////////////////////////////////////////
	// read and validate the configuration file from the command line
	////////////////////////////////////////////////////////////
	
	const QJsonObject jsonConfig = QJsonFactory::readJson(configFile);
	
	if (!schemaChecker.validate(jsonConfig))
	{
		for (std::list<std::string>::const_iterator i = schemaChecker.getMessages().begin(); i != schemaChecker.getMessages().end(); ++i)
		{
			std::cout << *i << std::endl;
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
