// stdlib includes
#include <iterator>
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
	_currentPath.append("[root]");

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
		else if (attribute == "minLength")
			checkMinLength(value, attributeValue);
		else if (attribute == "maxLength")
			checkMaxLength(value, attributeValue);
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
			|| attribute == "defaultProperties" || attribute == "propertyOrder" || attribute == "append" || attribute == "step" || attribute == "access" || attribute == "options" || attribute == "script")
 			; // nothing to do.
		else
		{
			// no check function defined for this attribute
			_error = true;
			setMessage("No check function defined for attribute " + attribute);
			continue;
		}
	}
}

void QJsonSchemaChecker::setMessage(const QString & message)
{
	_messages.append(_currentPath.join("") +": "+message);
}

const QStringList & QJsonSchemaChecker::getMessages() const
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
		setMessage(type + " expected");
	}
}

void QJsonSchemaChecker::checkProperties(const QJsonObject & value, const QJsonObject & schema)
{
	for (QJsonObject::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		QString property = i.key();

		const QJsonValue  & propertyValue = i.value();

		_currentPath.append("." + property);
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
		_currentPath.removeLast();
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
			_currentPath.append("." + property);
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
			_currentPath.removeLast();
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
		setMessage("value is too small (minimum=" + schema.toString() + ")");
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
		setMessage("value is too large (maximum=" + schema.toString() + ")");
	}
}

void QJsonSchemaChecker::checkMinLength(const QJsonValue & value, const QJsonValue & schema)
{
	if (!value.isString())
	{
		// only for Strings
		_error = true;
		setMessage("minLength check only for string fields");
		return;
	}

	if (value.toString().size() < schema.toInt())
	{
		_error = true;
		setMessage("value is too short (minLength=" + schema.toString() + ")");
	}
}

void QJsonSchemaChecker::checkMaxLength(const QJsonValue & value, const QJsonValue & schema)
{
	if (!value.isString())
	{
		// only for Strings
		_error = true;
		setMessage("maxLength check only for string fields");
		return;
	}

	if (value.toString().size() > schema.toInt())
	{
		_error = true;
		setMessage("value is too long (maxLength=" + schema.toString() + ")");
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
		_currentPath.append("[" + QString::number(i) + "]");
		validate(jArray[i], schema);
		_currentPath.removeLast();
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
		setMessage("array is too large (minimum=" + QString::number(minimum) + ")");
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
		setMessage("array is too large (maximum=" + QString::number(maximum) + ")");
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
	QJsonDocument doc(schema.toArray());
	QString strJson(doc.toJson(QJsonDocument::Compact));
	setMessage("Unknown enum value (allowed values are: " + schema.toString() + strJson+ ")");
}
