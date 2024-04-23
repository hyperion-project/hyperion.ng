//project include
#include <utils/JsonUtils.h>

// util includes
#include <utils/jsonschema/QJsonSchemaChecker.h>

//qt includes
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStringList>

namespace JsonUtils {

	QPair<bool, QStringList> readFile(const QString& path, QJsonObject& obj, Logger* log, bool ignError)
	{
		QString data;
		if(!FileUtils::readFile(path, data, log, ignError))
		{
			return qMakePair(false, QStringList(QString("Error reading file: %1").arg(path)));
		}

		QPair<bool, QStringList> parsingResult = JsonUtils::parse(path, data, obj, log);
		return parsingResult;
	}

	bool readSchema(const QString& path, QJsonObject& obj, Logger* log)
	{
		QJsonObject schema;
		if(!readFile(path, schema, log).first)
			return false;

		if(!resolveRefs(schema, obj, log))
			return false;

		return true;
	}

	QPair<bool, QStringList> parse(const QString& path, const QString& data, QJsonObject& obj, Logger* log)
	{
		QJsonDocument doc;
		QPair<bool, QStringList> parsingResult = JsonUtils::parse(path, data, doc, log);
		obj = doc.object();
		return parsingResult;
	}

	QPair<bool, QStringList> parse(const QString& path, const QString& data, QJsonArray& arr, Logger* log)
	{
		QJsonDocument doc;

		QPair<bool, QStringList> parsingResult = JsonUtils::parse(path, data, doc, log);
		arr = doc.array();
		return parsingResult;
	}

	QPair<bool, QStringList> parse(const QString& path, const QString& data, QJsonDocument& doc, Logger* log)
	{
		QStringList errorList;

		QJsonParseError error;
		doc = QJsonDocument::fromJson(data.toUtf8(), &error);

		if (error.error != QJsonParseError::NoError)
		{
			qDebug() << "error.offset: " << error.offset;

			int errorLine = 1;
			int errorColumn = 1;

			int lastNewlineIndex = data.lastIndexOf("\n", error.offset - 1);
			if (lastNewlineIndex != -1)
			{
				errorColumn = error.offset - lastNewlineIndex ;
			}
			errorLine += data.left(error.offset).count('\n');

			const QString errorMessage = QString("JSON parse error: %1, line: %2, column: %3, Data: '%4'")
											 .arg(error.errorString())
											 .arg(errorLine)
											 .arg(errorColumn)
											 .arg(data);
			errorList.push_back(errorMessage);
			Error(log, "%s", QSTRING_CSTR(errorMessage));

			return qMakePair(false, errorList);
		}
		return qMakePair(true, errorList);
	}

	QPair<bool, QStringList> validate(const QString& file, const QJsonObject& json, const QString& schemaPath, Logger* log)
	{
		// get the schema data
		QJsonObject schema;

		QPair<bool, QStringList> readResult = readFile(schemaPath, schema, log);
		if(!readResult.first)
		{
			return readResult;
		}

		QPair<bool, QStringList> validationResult = validate(file, json, schema, log);
		return validationResult;
	}

	QPair<bool, QStringList> validate(const QString& file, const QJsonObject& json, const QJsonObject& schema, Logger* log)
	{
		QStringList errorList;

		QJsonSchemaChecker schemaChecker;
		schemaChecker.setSchema(schema);
		if (!schemaChecker.validate(json).first)
		{
			const QStringList &errors = schemaChecker.getMessages();
			for (const auto& error : errors)
			{
				QString errorMessage = QString("JSON parse error: %1")
									   .arg(error);
				errorList.push_back(errorMessage);
				Error(log, "%s", QSTRING_CSTR(errorMessage));
			}
			return qMakePair(false, errorList);
		}
		return qMakePair(true, errorList);
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
				obj.insert(attribute, attributeValue);
			}
		}
		return true;
	}
};
