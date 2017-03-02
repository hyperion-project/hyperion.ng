#pragma once

#include <iostream>
#include <stdexcept>

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
		QJsonObject schemaTree = readSchema(schema);
		QJsonObject configTree = readConfig(config);

		// create the validator
		QJsonSchemaChecker schemaChecker;
		schemaChecker.setSchema(schemaTree);

		bool valid = schemaChecker.validate(configTree);
		
		QStringList messages = schemaChecker.getMessages();
		for (int i = 0; i < messages.size(); ++i)
		{
			std::cout << messages[i].toStdString() << std::endl;
		}
		
		if (!valid)
		{
			std::cerr << "Validation failed for configuration file: " << config.toStdString() << std::endl;
			return -3;
		}

		json = configTree;
		return 0;
	}
	
	static QJsonObject readConfig(const QString& path)
	{
		QFile file(path);
		QJsonParseError error;

		if (!file.open(QIODevice::ReadOnly))
		{
			throw std::runtime_error(QString("Configuration file not found: '" + path + "' ("  +  file.errorString() + ")").toStdString());
		}

		QString config = QString(file.readAll());
		config.remove(QRegularExpression("([^:]?\\/\\/.*)"));
		
		QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8(), &error);
		file.close();

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

			throw std::runtime_error (
				QString("Failed to parse configuration: " + error.errorString() + " at Line: " + QString::number(errorLine) + ", Column: " + QString::number(errorColumn)).toStdString()
			);
		}

		return doc.object();
	}

	static QJsonObject readSchema(const QString& path)
	{
		QFile schemaData(path);
		QJsonParseError error;

		if (!schemaData.open(QIODevice::ReadOnly))
		{
			throw std::runtime_error(QString("Schema not found: '" + path + "' (" + schemaData.errorString() + ")").toStdString());
		}

		QByteArray schema = schemaData.readAll();
		QJsonDocument doc = QJsonDocument::fromJson(schema, &error);
		schemaData.close();
		
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

			throw std::runtime_error(QString("ERROR: Json schema wrong: " + error.errorString() + " at Line: " + QString::number(errorLine)
				                     + ", Column: " + QString::number(errorColumn)).toStdString()
			);
		}

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
