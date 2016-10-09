#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>

// JSON-Schema includes
#include <utils/jsonschema/QJsonSchemaChecker.h>

#include <QFile>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>

class QJsonFactory
{
public:

	static int load(const QString& schema, const QString& config, QJsonObject& json)
	{
		// Load the schema and the config trees
		QJsonObject schemaTree = readJson(schema);
		QJsonObject configTree = readJson(config);

		// create the validator
		QJsonSchemaChecker schemaChecker;
		schemaChecker.setSchema(schemaTree);

		bool valid = schemaChecker.validate(configTree);
		
		for (std::list<std::string>::const_iterator i = schemaChecker.getMessages().begin(); i != schemaChecker.getMessages().end(); ++i)
		{
			std::cout << *i << std::endl;
		}
		
		if (!valid)
		{
			std::cerr << "Validation failed for configuration file: " << config.toStdString() << std::endl;
			return -3;
		}

		json = configTree;
		return 0;
	}
	
	static QJsonObject readJson(const QString& path)
	{
		QFile file(path);
		QJsonParseError error;
		
		if (!file.open(QIODevice::ReadOnly))
		{
			std::stringstream sstream;
			sstream << "Configuration file not found: " << file.errorString().toStdString();
			throw std::runtime_error(sstream.str());
		}

		QString config = QString(file.readAll());
		config.remove(QRegularExpression("([^:]?\\/\\/.*)"));
		
		QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8(), &error);
		
		if (error.error != QJsonParseError::NoError)
		{
			// report to the user the failure and their locations in the document.
			int errorLine(0), errorColumn(0);
			
			for( int i=0, count=qMin( error.offset,config.size()); i<count; ++i )
			{
				++errorColumn;
				if(config.at(i) == '\n' )
				{
					errorColumn = 0;
					++errorLine;
				}
			}
			
			std::stringstream sstream;
			sstream << "Failed to parse configuration: " << error.errorString().toStdString() << " at Line: " << errorLine << ", Column: " << errorColumn;
			throw std::runtime_error(sstream.str());
		}
		
		file.close();
		return doc.object();
	}

	static void writeJson(const QString& filename, QJsonObject& jsonTree)
	{
		QJsonDocument doc;
		doc.setObject(jsonTree);
		QByteArray configData = doc.toJson(QJsonDocument::Indented);
		
		QFile configFile(filename);
		configFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
		configFile.write(configData);
		configFile.close();
	}
};
