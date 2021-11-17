// QT includes
#include <QResource>
#include <QDebug>

// JsonSchema includes
#include <utils/jsonschema/QJsonFactory.h>

// hyperion includes
#include <hyperion/LedString.h>
#include "HyperionConfig.h"

bool loadConfig(const QString & configFile, bool correct, bool ignore)
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
		if (!schemaChecker.validate(jsonConfig).first)
		{
			QStringList schemaErrors = schemaChecker.getMessages();
			for (auto & schemaError : schemaErrors)
			{
				qDebug() << "config write validation: " << schemaError;
			}

			qDebug() << "FAILED";
			exit(1);
			return false;
		}
	}
	else
	{
		jsonConfig = schemaChecker.getAutoCorrectedConfig(jsonConfig, ignore); // The second parameter is to ignore the "required" keyword in hyperion schema
		QJsonFactory::writeJson(configFile, jsonConfig);
	}

	return true;
}

void usage()
{
	qDebug() << "Missing required configuration file to test";
	qDebug() << "Usage: test_configfile <option> [configfile]";
	qDebug() << "<option>:";
	qDebug() << "\t--ac - for json auto correction";
	qDebug() << "\t--ac-ignore-required - for json auto correction without paying attention 'required' keyword in hyperion schema";
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

	if (option == "--ac" || option == "--ac-ignore-required")
	{
		if (argc > 2)
			configFile = argv[2];
		else
		{
			usage();
			return 0;
		}
	}
	else configFile = argv[1];

	qDebug() << "Configuration file selected: " << configFile;
	qDebug() << "Attemp to load...";
	try
	{
		if (loadConfig(configFile, (option == "--ac" || option == "--ac-ignore-required"), option == "--ac-ignore-required"))
			qDebug() << "PASSED";
		return 0;
	}
	catch (std::runtime_error& exception)
	{
		qDebug() << "FAILED";
		qDebug() << exception.what();
	}

	return 1;
}
