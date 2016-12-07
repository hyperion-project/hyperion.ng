// stdlib includes
#include <iterator>
#include <sstream>
#include <algorithm>
#include <math.h>

// Utils-Jsonschema includes
#include <utils/jsonschema/QJsonSchemaChecker.h>

QJsonSchemaChecker::QJsonSchemaChecker()
{
	// empty
}

QJsonSchemaChecker::~QJsonSchemaChecker()
{
	// empty
}

bool QJsonSchemaChecker::setSchema(const QJsonObject & schema)
{
	_qSchema = schema;

	// TODO: check the schema

	return true;
}

bool QJsonSchemaChecker::validate(const QJsonObject & value, bool ignoreRequired)
{
	// initialize state
	_ignoreRequired = ignoreRequired;
	_error = false;
	_messages.clear();
	_currentPath.clear();
	_currentPath.push_back("[root]");

	// validate
	validate(value, _qSchema);

	return !_error;
}

void QJsonSchemaChecker::validate(const QJsonValue & value, const QJsonObject &schema)
{
	// check the current json value
	for (QJsonObject::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		QString attribute = i.key();
		const QJsonValue & attributeValue = *i;

		if (attribute == "type")
			checkType(value, attributeValue);
		else if (attribute == "properties")
		{
			if (value.isObject())
				checkProperties(value.toObject(), attributeValue.toObject());
			else
			{
				_error = true;
				setMessage("properties attribute is only valid for objects");
				continue;
			}
		}
		else if (attribute == "additionalProperties")
		{
			if (value.isObject())
			{
				// ignore the properties which are handled by the properties attribute (if present)
				QStringList ignoredProperties;
				if (schema.contains("properties")) {
					const QJsonObject & props = schema["properties"].toObject();
					ignoredProperties = props.keys();
				}

				checkAdditionalProperties(value.toObject(), attributeValue, ignoredProperties);
			}
			else
			{
				_error = true;
				setMessage("additional properties attribute is only valid for objects");
				continue;
			}
		}
		else if (attribute == "minimum")
			checkMinimum(value, attributeValue);
		else if (attribute == "maximum")
			checkMaximum(value, attributeValue);
		else if (attribute == "items")
		{
			if (value.isArray())
				checkItems(value, attributeValue.toObject());
			else
			{
				_error = true;
				setMessage("items only valid for arrays");
				continue;
			}
		}
		else if (attribute == "minItems")
			checkMinItems(value, attributeValue);
		else if (attribute == "maxItems")
			checkMaxItems(value, attributeValue);
		else if (attribute == "uniqueItems")
			checkUniqueItems(value, attributeValue);
		else if (attribute == "enum")
			checkEnum(value, attributeValue);
 		else if (attribute == "required")
 			; // nothing to do. value is present so always oke
 		else if (attribute == "id")
 			; // references have already been collected
 		else if (attribute == "title" || attribute == "description"  || attribute == "default" || attribute == "format"
			|| attribute == "defaultProperties" || attribute == "propertyOrder" || attribute == "append" || attribute == "step" || attribute == "access")
 			; // nothing to do.
		else
		{
			// no check function defined for this attribute
			setMessage(std::string("No check function defined for attribute ") + attribute.toStdString());
			continue;
		}
	}
}

void QJsonSchemaChecker::setMessage(const std::string & message)
{
	std::ostringstream oss;
	std::copy(_currentPath.begin(), _currentPath.end(), std::ostream_iterator<std::string>(oss, ""));
	oss << ": " << message;
	_messages.push_back(oss.str());
}

const std::list<std::string> & QJsonSchemaChecker::getMessages() const
{
	return _messages;
}

void QJsonSchemaChecker::checkType(const QJsonValue & value, const QJsonValue & schema)
{
	QString type = schema.toString();

	bool wrongType = false;
	if (type == "string")
		wrongType = !value.isString();
	else if (type == "number")
		wrongType = !value.isDouble();
	else if (type == "integer")
		wrongType = (rint(value.toDouble()) != value.toDouble());
	else if (type == "double")
		wrongType = !value.isDouble();
	else if (type == "boolean")
		wrongType = !value.isBool();
	else if (type == "object")
		wrongType = !value.isObject();
	else if (type == "array")
		wrongType = !value.isArray();
	else if (type == "null")
		wrongType = !value.isNull();
	else if (type == "enum")
		wrongType = !value.isString();
	else if (type == "any")
		wrongType = false;
//	else
//		assert(false);

	if (wrongType)
	{
		_error = true;
		setMessage(type.toStdString() + " expected");
	}
}

void QJsonSchemaChecker::checkProperties(const QJsonObject & value, const QJsonObject & schema)
{
	for (QJsonObject::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		QString property = i.key();

		const QJsonValue  & propertyValue = i.value();

		_currentPath.push_back(std::string(".") + property.toStdString());
		QJsonObject::const_iterator required = propertyValue.toObject().find("required");

		if (value.contains(property))
		{
			validate(value[property], propertyValue.toObject());
		}
		else if (required != propertyValue.toObject().end() && required.value().toBool() && !_ignoreRequired)
		{
			_error = true;
			setMessage("missing member");
		}
		_currentPath.pop_back();
	}
}

void QJsonSchemaChecker::checkAdditionalProperties(const QJsonObject & value, const QJsonValue & schema, const QStringList & ignoredProperties)
{
	for (QJsonObject::const_iterator i = value.begin(); i != value.end(); ++i)
	{
		QString property = i.key();
		if (std::find(ignoredProperties.begin(), ignoredProperties.end(), property) == ignoredProperties.end())
		{
			// property has no property definition. check against the definition for additional properties
			_currentPath.push_back(std::string(".") + property.toStdString());
			if (schema.isBool())
			{
				if (schema.toBool() == false)
				{
					_error = true;
					setMessage("no schema definition");
				}
			}
			else
			{
				validate(value[property].toObject(), schema.toObject());
			}
			_currentPath.pop_back();
		}
	}
}

void QJsonSchemaChecker::checkMinimum(const QJsonValue & value, const QJsonValue & schema)
{
	if (!value.isDouble())
	{
		// only for numeric
		_error = true;
		setMessage("minimum check only for numeric fields");
		return;
	}

	if (value.toDouble() < schema.toDouble())
	{
		_error = true;
		std::ostringstream oss;
		oss << "value is too small (minimum=" << schema.toDouble() << ")";
		setMessage(oss.str());
	}
}

void QJsonSchemaChecker::checkMaximum(const QJsonValue & value, const QJsonValue & schema)
{
	if (!value.isDouble())
	{
		// only for numeric
		_error = true;
		setMessage("maximum check only for numeric fields");
		return;
	}

	if (value.toDouble() > schema.toDouble())
	{
		_error = true;
		std::ostringstream oss;
		oss << "value is too large (maximum=" << schema.toDouble() << ")";
		setMessage(oss.str());
	}
}

void QJsonSchemaChecker::checkItems(const QJsonValue & value, const QJsonObject & schema)
{
	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("items only valid for arrays");
		return;
	}

	QJsonArray jArray = value.toArray();
	for(int i = 0; i < jArray.size(); ++i)
	{
		// validate each item
		std::ostringstream oss;
		oss << "[" << i << "]";
		_currentPath.push_back(oss.str());
		validate(jArray[i], schema);
		_currentPath.pop_back();
	}
}

void QJsonSchemaChecker::checkMinItems(const QJsonValue & value, const QJsonValue & schema)
{
	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("minItems only valid for arrays");
		return;
	}

	int minimum = schema.toInt();

	QJsonArray jArray = value.toArray();
	if (static_cast<int>(jArray.size()) < minimum)
	{
		_error = true;
		std::ostringstream oss;
		oss << "array is too small (minimum=" << minimum << ")";
		setMessage(oss.str());
	}
}

void QJsonSchemaChecker::checkMaxItems(const QJsonValue & value, const QJsonValue & schema)
{
	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("maxItems only valid for arrays");
		return;
	}

	int maximum = schema.toInt();

	QJsonArray jArray = value.toArray();
	if (static_cast<int>(jArray.size()) > maximum)
	{
		_error = true;
		std::ostringstream oss;
		oss << "array is too large (maximum=" << maximum << ")";
		setMessage(oss.str());
	}
}

void QJsonSchemaChecker::checkUniqueItems(const QJsonValue & value, const QJsonValue & schema)
{
	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("uniqueItems only valid for arrays");
		return;
	}

	if (schema.toBool() == true)
	{
		// make sure no two items are identical

		QJsonArray jArray = value.toArray();
		for(int i = 0; i < jArray.size(); ++i)
		{
			for (int j = i+1; j < jArray.size(); ++j)
			{
				if (jArray[i] == jArray[j])
				{
					// found a value twice
					_error = true;
					setMessage("array must have unique values");
				}
			}
		}
	}
}

void QJsonSchemaChecker::checkEnum(const QJsonValue & value, const QJsonValue & schema)
{
	if (schema.isArray())
	{
		QJsonArray jArray = schema.toArray();
		for(int i = 0; i < jArray.size(); ++i)
		{
			if (jArray[i] == value)
			{
				// found enum value. done.
				return;
			}
		}
	}

	// nothing found
	_error = true;
	std::ostringstream oss;
	oss << "Unknown enum value (allowed values are: " << schema.toString().toStdString();
	QJsonDocument doc(schema.toArray());
	QString strJson(doc.toJson(QJsonDocument::Compact));
	oss << strJson.toStdString() << ")";
	setMessage(oss.str());
}
