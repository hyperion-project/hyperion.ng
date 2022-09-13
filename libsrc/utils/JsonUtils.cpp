//project include
#include <utils/JsonUtils.h>

// util includes
#include <utils/jsonschema/QJsonSchemaChecker.h>

//qt includes
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonParseError>

namespace JsonUtils {

	bool readFile(const QString& path, QJsonObject& obj, Logger* log, bool ignError)
	{
		QString data;
		if(!FileUtils::readFile(path, data, log, ignError))
			return false;

		if(!parse(path, data, obj, log))
			return false;

		return true;
	}

	bool readSchema(const QString& path, QJsonObject& obj, Logger* log)
	{
		QJsonObject schema;
		if(!readFile(path, schema, log))
			return false;

		if(!resolveRefs(schema, obj, log))
			return false;

		return true;
	}

	bool parse(const QString& path, const QString& data, QJsonObject& obj, Logger* log)
	{
		QJsonDocument doc;
		if(!parse(path, data, doc, log))
			return false;

		obj = doc.object();
		return true;
	}

	bool parse(const QString& path, const QString& data, QJsonArray& arr, Logger* log)
	{
		QJsonDocument doc;
		if(!parse(path, data, doc, log))
			return false;

		arr = doc.array();
		return true;
	}

	bool parse(const QString& path, const QString& data, QJsonDocument& doc, Logger* log)
	{
		//remove Comments in data
		QString cleanData = data;
		//cleanData .remove(QRegularExpression("([^:]?\\/\\/.*)"));

		QJsonParseError error;
		doc = QJsonDocument::fromJson(cleanData.toUtf8(), &error);

		if (error.error != QJsonParseError::NoError)
		{
			// report to the user the failure and their locations in the document.
			int errorLine(0), errorColumn(0);

			for( int i=0, count=qMin( error.offset,cleanData.size()); i<count; ++i )
			{
				++errorColumn;
				if(data.at(i) == '\n' )
				{
					errorColumn = 0;
					++errorLine;
				}
			}
			Error(log, "Failed to parse json data from %s: Error: %s at Line: %i, Column: %i, Data: '%s'", QSTRING_CSTR(path), QSTRING_CSTR(error.errorString()), errorLine, errorColumn, QSTRING_CSTR(data));
			return false;
		}
		return true;
	}

	bool validate(const QString& file, const QJsonObject& json, const QString& schemaPath, Logger* log)
	{
		// get the schema data
		QJsonObject schema;
		if(!readFile(schemaPath, schema, log))
			return false;

		if(!validate(file, json, schema, log))
			return false;
		return true;

	}

	bool validate(const QString& file, const QJsonObject& json, const QJsonObject& schema, Logger* log)
	{
		QJsonSchemaChecker schemaChecker;
		schemaChecker.setSchema(schema);
		if (!schemaChecker.validate(json).first)
		{
			const QStringList & errors = schemaChecker.getMessages();
			for (auto & error : errors)
			{
				Error(log, "While validating schema against json data of '%s':%s", QSTRING_CSTR(file), QSTRING_CSTR(error));
			}
			return false;
		}
		return true;
	}

	bool write(const QString& filename, const QJsonObject& json, Logger* log)
	{
		QJsonDocument doc;

		doc.setObject(json);
		QByteArray data = doc.toJson(QJsonDocument::Indented);

		if(!FileUtils::writeFile(filename, data, log))
			return false;

		return true;
	}

	bool resolveRefs(const QJsonObject& schema, QJsonObject& obj, Logger* log)
	{
		for (QJsonObject::const_iterator i = schema.begin(); i != schema.end(); ++i)
		{
			QString attribute = i.key();
			const QJsonValue & attributeValue = *i;

			if (attribute == "$ref" && attributeValue.isString())
			{
				if(!readSchema(":/" + attributeValue.toString(), obj, log))
				{
					Error(log,"Error while getting schema ref: %s",QSTRING_CSTR(QString(":/" + attributeValue.toString())));
					return false;
				}
			}
			else if (attributeValue.isObject())
				obj.insert(attribute, resolveRefs(attributeValue.toObject(), obj, log));
			else
			{
				//qDebug() <<"ADD ATTR:VALUE"<<attribute<<attributeValue;
				obj.insert(attribute, attributeValue);
			}
		}
		return true;
	}
};
