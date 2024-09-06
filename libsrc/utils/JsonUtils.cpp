//project include
#include <utils/JsonUtils.h>

// util includes
#include <utils/jsonschema/QJsonSchemaChecker.h>

//qt includes
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStringList>

namespace JsonUtils {

QPair<bool, QStringList> readFile(const QString& path, QJsonObject& obj, Logger* log, bool ignError)
{
	QJsonValue value(obj);

	QPair<bool, QStringList> result = readFile(path, value,log, ignError);
	obj = value.toObject();
	return result;
}

QPair<bool, QStringList> readFile(const QString& path, QJsonValue& obj, Logger* log, bool ignError)
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
	{
		return false;
	}

	if(!resolveRefs(schema, obj, log))
	{
		return false;
	}

	return true;
}

QPair<bool, QStringList> parse(const QString& path, const QString& data, QJsonObject& obj, Logger* log)
{
	QJsonValue value(obj);

	QPair<bool, QStringList> result = JsonUtils::parse(path, data, value, log);
	obj = value.toObject();
	return result;
}

QPair<bool, QStringList> parse(const QString& path, const QString& data, QJsonArray& arr, Logger* log)
{
	QJsonValue value(arr);

	QPair<bool, QStringList> result = JsonUtils::parse(path, data, value, log);
	arr = value.toArray();
	return result;
}

QPair<bool, QStringList> parse(const QString& path, const QString& data, QJsonValue& value, Logger* log)
{
	QJsonDocument doc;
	QPair<bool, QStringList> parsingResult = JsonUtils::parse(path, data, doc, log);
	value = doc.object();
	return parsingResult;
}

QPair<bool, QStringList> parse(const QString& path, const QString& data, QJsonDocument& doc, Logger* log)
{
	QStringList errorList;

	QJsonParseError error;
	doc = QJsonDocument::fromJson(data.toUtf8(), &error);

	if (error.error != QJsonParseError::NoError)
	{
		int errorLine = 1;
		int errorColumn = 1;

		int lastNewlineIndex = data.lastIndexOf("\n", error.offset - 1);
		if (lastNewlineIndex != -1)
		{
			errorColumn = error.offset - lastNewlineIndex ;
		}
		errorLine += data.left(error.offset).count('\n');

		const QString errorMessage = QString("JSON parse error @(%1): %2, line: %3, column: %4, Data: '%5'")
									 .arg(path)
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

QPair<bool, QStringList> validate(const QString& file, const QJsonValue& json, const QString& schemaPath, Logger* log)
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

QPair<bool, QStringList> validate(const QString& file, const QJsonValue& json, const QJsonObject& schema, Logger* log)
{
	QStringList errorList;

	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schema);
	if (!schemaChecker.validate(json).first)
	{
		const QStringList &errors = schemaChecker.getMessages();
		for (const auto& error : errors)
		{
			QString errorMessage = QString("JSON parse error @(%1) -  %2")
								   .arg(file, error);
			errorList.push_back(errorMessage);
			Error(log, "%s", QSTRING_CSTR(errorMessage));
		}
		return qMakePair(false, errorList);
	}
	return qMakePair(true, errorList);
}

QPair<bool, QStringList> correct(const QString& file, QJsonValue& json, const QJsonObject& schema, Logger* log)
{
	bool wasCorrected {false};
	QStringList corrections;
	QJsonValue correctJson;

	QJsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schema);
	if (!schemaChecker.validate(json).first)
	{
		Warning(log, "Fixing JSON data!");
		json = schemaChecker.getAutoCorrectedConfig(json);
		wasCorrected = true;

		const QStringList &correctionMessages = schemaChecker.getMessages();
		for (const auto& correction : correctionMessages)
		{
			QString message = QString("JSON fix @(%1) -  %2")
							  .arg(file, correction);
			corrections.push_back(message);
			Warning(log, "%s", QSTRING_CSTR(message));
		}
	}
	return qMakePair(wasCorrected, corrections);
}

bool write(const QString& filename, const QJsonObject& json, Logger* log)
{
	QJsonDocument doc;

	doc.setObject(json);
	QByteArray data = doc.toJson(QJsonDocument::Indented);

	return FileUtils::writeFile(filename, data, log);
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
		{
			obj.insert(attribute, resolveRefs(attributeValue.toObject(), obj, log));
		}
		else
		{
			obj.insert(attribute, attributeValue);
		}
	}
	return true;
}

QString jsonValueToQString(const QJsonValue &value, QJsonDocument::JsonFormat format)
{
	switch (value.type()) {
	case QJsonValue::Object:
	{
		return QJsonDocument(value.toObject()).toJson(format);
	}
	case QJsonValue::Array:
	{
		return QJsonDocument(value.toArray()).toJson(format);
	}
	case QJsonValue::String:
	case QJsonValue::Double:
	case QJsonValue::Bool:
	{
		return value.toString();
	}
	case QJsonValue::Null:
	{
		return "Null";
	}
	default:
	break;
	}
	return QString();
}

QJsonObject mergeJsonObjects(const QJsonObject &obj1, const QJsonObject &obj2, bool overrideObj1)
{
	QJsonObject result = obj1;

	for (auto it = obj2.begin(); it != obj2.end(); ++it) {
		if (result.contains(it.key())) {
			if (overrideObj1)
			{
				result[it.key()] = it.value();
			}
		} else {
			result[it.key()] = it.value();
		}
	}

	return result;
}
};
