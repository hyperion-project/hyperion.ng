// stdlib includes
#include <iterator>
#include <algorithm>
#include <cmath>

// Utils-Jsonschema includes
#include <utils/jsonschema/QJsonSchemaChecker.h>
#include <utils/jsonschema/QJsonUtils.h>

QJsonSchemaChecker::QJsonSchemaChecker() :
	_ignoreRequired(false),
	_error(false),
	_schemaError(false)
{
	// empty
}

QJsonSchemaChecker::~QJsonSchemaChecker()
{
	// empty
}

bool QJsonSchemaChecker::setSchema(const QJsonObject& schema)
{
	_qSchema = schema;

	// TODO: check the schema

	return true;
}

void QJsonSchemaChecker::setMessage(const QString& message)
{
	_messages.append(_currentPath.join("") + ": " + message);
}

QStringList QJsonSchemaChecker::getMessages() const
{
	return _messages;
}

QPair<bool, bool> QJsonSchemaChecker::validate(const QJsonObject& value, bool ignoreRequired)
{
	// initialize state
	_ignoreRequired = ignoreRequired;
	_error = false;
	_schemaError = false;
	_messages.clear();
	_currentPath.clear();
	_currentPath.append("[root]");

	// validate
	validate(value, _qSchema);

	return QPair<bool, bool>(!_error, !_schemaError);
}

QJsonObject QJsonSchemaChecker::getAutoCorrectedConfig(const QJsonObject& value, bool ignoreRequired)
{
	_ignoreRequired = ignoreRequired;
	QStringList sequence = QStringList() << "remove" << "modify" << "create";
	_error = false;
	_schemaError = false;
	_messages.clear();
	_autoCorrected = value;

	for (const QString& correct : qAsConst(sequence))
	{
		_correct = correct;
		_currentPath.clear();
		_currentPath.append("[root]");
		validate(_autoCorrected, _qSchema);
	}

	return _autoCorrected;
}

void QJsonSchemaChecker::validate(const QJsonValue& value, const QJsonObject& schema)
{
	// check the current json value
	for (QJsonObject::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		QString attribute = i.key();
		const QJsonValue& attributeValue = *i;

		QJsonObject::const_iterator defaultValue = schema.find("default");

		if (attribute == "type")
			checkType(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
		else if (attribute == "properties")
		{
			if (value.isObject())
				checkProperties(value.toObject(), attributeValue.toObject());
			else
			{
				_schemaError = true;
				setMessage("properties attribute is only valid for objects");
				continue;
			}
		}
		else if (attribute == "dependencies")
		{
			if (value.isObject())
				checkDependencies(value.toObject(), attributeValue.toObject());
			else
			{
				_schemaError = true;
				setMessage("dependencies attribute is only valid for objects");
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
					const QJsonObject& props = schema["properties"].toObject();
					ignoredProperties = props.keys();
				}

				checkAdditionalProperties(value.toObject(), attributeValue, ignoredProperties);
			}
			else
			{
				_schemaError = true;
				setMessage("additional properties attribute is only valid for objects");
				continue;
			}
		}
		else if (attribute == "minimum")
			checkMinimum(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
		else if (attribute == "maximum")
			checkMaximum(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
		else if (attribute == "minLength")
			checkMinLength(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
		else if (attribute == "maxLength")
			checkMaxLength(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
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
			checkMinItems(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
		else if (attribute == "maxItems")
			checkMaxItems(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
		else if (attribute == "uniqueItems")
			checkUniqueItems(value, attributeValue);
		else if (attribute == "enum")
			checkEnum(value, attributeValue, (defaultValue != schema.end() ? *defaultValue : QJsonValue(QJsonValue::Null)));
		else if (attribute == "required")
			; // nothing to do. value is present so always oke
		else if (attribute == "id")
			; // references have already been collected
		else if (attribute == "title" || attribute == "description" || attribute == "default" || attribute == "format"
			|| attribute == "defaultProperties" || attribute == "propertyOrder" || attribute == "append" || attribute == "step"
			|| attribute == "access" || attribute == "options" || attribute == "script" || attribute == "allowEmptyArray" || attribute == "comment"
			|| attribute == "watch" || attribute == "template")
			; // nothing to do.
		else
		{
			// no check function defined for this attribute
			_schemaError = true;
			setMessage("No check function defined for attribute " + attribute);
			continue;
		}
	}
}

void QJsonSchemaChecker::checkType(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
{
	QString type = schema.toString();

	bool wrongType = false;
	if (type == "string")
		wrongType = !value.isString();
	else if (type == "number")
		wrongType = !value.isDouble();
	else if (type == "integer")
	{
		if (value.isDouble()) //check if value type not boolean (true = 1 && false = 0)
		{
			double valueIntegratlPart;
			double valueFractionalPart = std::modf(value.toDouble(), &valueIntegratlPart);
			if (valueFractionalPart > std::numeric_limits<double>::epsilon())
			{
				wrongType = true;
			}
		}
		else
			wrongType = true;
	}
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

	if (wrongType)
	{
		_error = true;

		if (_correct == "modify")
			QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue);


		if (_correct == "")
			setMessage(type + " expected");
	}
}

void QJsonSchemaChecker::checkProperties(const QJsonObject& value, const QJsonObject& schema)
{
	for (QJsonObject::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		QString property = i.key();

		const QJsonValue& propertyValue = *i;

		_currentPath.append("." + property);

		if (value.contains(property))
		{
			validate(value[property], propertyValue.toObject());
		}
		else if (!verifyDeps(property, value, schema))
		{
			bool isRequired = propertyValue.toObject().value("required").toBool(false);
			if (isRequired && !_ignoreRequired)
			{
				_error = true;

				if (_correct == "create")
				{
					QJsonUtils::modify(_autoCorrected, _currentPath, QJsonUtils::create(propertyValue, _ignoreRequired), property);
					setMessage("Create property: " + property + " with value: " + QJsonUtils::getDefaultValue(propertyValue));
				}

				if (_correct == "")
				{
					setMessage("missing member");
				}
			}
			else if (_correct == "create" && _ignoreRequired)
			{
				QJsonUtils::modify(_autoCorrected, _currentPath, QJsonUtils::create(propertyValue, _ignoreRequired), property);
			}
		}

		_currentPath.removeLast();
	}
}

bool QJsonSchemaChecker::verifyDeps(const QString& property, const QJsonObject& value, const QJsonObject& schema)
{
	if (schema[property].toObject().contains("options") && (schema[property].toObject())["options"].toObject().contains("dependencies"))
	{
		const QJsonObject& depends = ((schema[property].toObject())["options"].toObject())["dependencies"].toObject();

		const QStringList dependsKeys = depends.keys();
		if (!dependsKeys.isEmpty())
		{
			const QString firstName = dependsKeys.constFirst();
			if (value.contains(firstName))
			{
				if (value[firstName] != depends[firstName])
					return true;
			}
		}
	}
	return false;
}

void QJsonSchemaChecker::checkDependencies(const QJsonObject& value, const QJsonObject& schema)
{
	for (QJsonObject::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		QString property = i.key();
		const QJsonValue& propertyValue = *i;

		if (propertyValue.toObject().contains("properties"))
		{
			_currentPath.append("." + property);

			const QJsonObject& dependencies = propertyValue.toObject()["properties"].toObject();

			bool valid = false;
			for (QJsonObject::const_iterator d = dependencies.begin(); d != dependencies.end(); ++d)
			{
				QString dependency = d.key();
				const QJsonValue& dependencyValue = *d;

				if (dependencyValue.toObject()["enum"].isArray())
				{
					QJsonArray jArray = dependencyValue.toObject()["enum"].toArray();
					for (int a = 0; a < jArray.size(); ++a)
					{
						if (value[dependency] == jArray[a])
						{
							valid = true;
							break;
						}
						else
							valid = false;
					}
				}
				else
					valid = (value[dependency] == dependencyValue.toObject()["enum"]);
			}

			if (value.contains(property) && !valid)
			{
				_error = true;

				if (_correct == "remove")
				{
					QJsonUtils::modify(_autoCorrected, _currentPath);
					setMessage("Removed property: " + property);
				}

				if (_correct == "")
					setMessage("Property not required");
			}

			_currentPath.removeLast();
		}
	}
}

void QJsonSchemaChecker::checkAdditionalProperties(const QJsonObject& value, const QJsonValue& schema, const QStringList& ignoredProperties)
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

					if (_correct == "remove")
					{
						QJsonUtils::modify(_autoCorrected, _currentPath);
						setMessage("Removed property: " + property);
					}

					if (_correct == "")
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

void QJsonSchemaChecker::checkMinimum(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
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

		if (_correct == "modify")
		{
			(defaultValue != QJsonValue::Null) ?
				QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue) :
				QJsonUtils::modify(_autoCorrected, _currentPath, schema);
			setMessage("Correct too small value: " + QString::number(value.toDouble()) + " to: " + QString::number(defaultValue.toDouble()));
		}

		if (_correct == "")
			setMessage("value is too small (minimum=" + QString::number(schema.toDouble()) + ")");
	}
}

void QJsonSchemaChecker::checkMaximum(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
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

		if (_correct == "modify")
		{
			(defaultValue != QJsonValue::Null) ?
				QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue) :
				QJsonUtils::modify(_autoCorrected, _currentPath, schema);
			setMessage("Correct too large value: " + QString::number(value.toDouble()) + " to: " + QString::number(defaultValue.toDouble()));
		}

		if (_correct == "")
			setMessage("value is too large (maximum=" + QString::number(schema.toDouble()) + ")");
	}
}

void QJsonSchemaChecker::checkMinLength(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
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

		if (_correct == "modify")
		{
			(defaultValue != QJsonValue::Null) ?
				QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue) :
				QJsonUtils::modify(_autoCorrected, _currentPath, schema);
			setMessage("Correct too short value: " + value.toString() + " to: " + defaultValue.toString());
		}
		if (_correct == "")
			setMessage("value is too short (minLength=" + QString::number(schema.toInt()) + ")");
	}
}

void QJsonSchemaChecker::checkMaxLength(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
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

		if (_correct == "modify")
		{
			(defaultValue != QJsonValue::Null) ?
				QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue) :
				QJsonUtils::modify(_autoCorrected, _currentPath, schema);
			setMessage("Correct too long value: " + value.toString() + " to: " + defaultValue.toString());
		}
		if (_correct == "")
			setMessage("value is too long (maxLength=" + QString::number(schema.toInt()) + ")");
	}
}

void QJsonSchemaChecker::checkItems(const QJsonValue& value, const QJsonObject& schema)
{
	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("items only valid for arrays");
		return;
	}

	QJsonArray jArray = value.toArray();

	if (_correct == "remove")
		if (jArray.isEmpty() && !schema.contains("allowEmptyArray"))
		{
			QJsonUtils::modify(_autoCorrected, _currentPath);
			setMessage("Remove empty array");
		}

	for (int i = 0; i < jArray.size(); ++i)
	{
		// validate each item
		_currentPath.append("[" + QString::number(i) + "]");
		validate(jArray[i], schema);
		_currentPath.removeLast();
	}
}

void QJsonSchemaChecker::checkMinItems(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
{
	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("minItems only valid for arrays");
		return;
	}

	QJsonArray jArray = value.toArray();
	if (jArray.size() < schema.toInt())
	{
		_error = true;

		if (_correct == "modify")
		{
			(defaultValue != QJsonValue::Null) ?
				QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue) :
				QJsonUtils::modify(_autoCorrected, _currentPath, schema);
			setMessage("Correct minItems: " + QString::number(jArray.size()) + " to: " + QString::number(defaultValue.toArray().size()));
		}

		if (_correct == "")
			setMessage("array is too small (minimum=" + QString::number(schema.toInt()) + ")");
	}
}

void QJsonSchemaChecker::checkMaxItems(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
{
	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("maxItems only valid for arrays");
		return;
	}

	QJsonArray jArray = value.toArray();
	if (jArray.size() > schema.toInt())
	{
		_error = true;

		if (_correct == "modify")
		{
			(defaultValue != QJsonValue::Null) ?
				QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue) :
				QJsonUtils::modify(_autoCorrected, _currentPath, schema);
			setMessage("Correct maxItems: " + QString::number(jArray.size()) + " to: " + QString::number(defaultValue.toArray().size()));
		}

		if (_correct == "")
			setMessage("array is too large (maximum=" + QString::number(schema.toInt()) + ")");
	}
}

void QJsonSchemaChecker::checkUniqueItems(const QJsonValue& value, const QJsonValue& schema)
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

		bool removeDuplicates = false;

		QJsonArray jArray = value.toArray();
		for (int i = 0; i < jArray.size(); ++i)
		{
			for (int j = i + 1; j < jArray.size(); ++j)
			{
				if (jArray[i] == jArray[j])
				{
					// found a value twice
					_error = true;
					removeDuplicates = true;

					if (_correct == "")
						setMessage("array must have unique values");
				}
			}
		}

		if (removeDuplicates && _correct == "modify")
		{
			QJsonArray uniqueItemsArray;

			for (int i = 0; i < jArray.size(); ++i)
				if (!uniqueItemsArray.contains(jArray[i]))
					uniqueItemsArray.append(jArray[i]);

			QJsonUtils::modify(_autoCorrected, _currentPath, uniqueItemsArray);
		}
	}
}

void QJsonSchemaChecker::checkEnum(const QJsonValue& value, const QJsonValue& schema, const QJsonValue& defaultValue)
{
	if (schema.isArray())
	{
		QJsonArray jArray = schema.toArray();
		for (int i = 0; i < jArray.size(); ++i)
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

	if (_correct == "modify")
	{
		(defaultValue != QJsonValue::Null) ?
			QJsonUtils::modify(_autoCorrected, _currentPath, defaultValue) :
			QJsonUtils::modify(_autoCorrected, _currentPath, schema.toArray().first());
		setMessage("Correct unknown enum value: " + value.toString() + " to: " + defaultValue.toString());
	}

	if (_correct == "")
	{
		QJsonDocument doc(schema.toArray());
		QString strJson(doc.toJson(QJsonDocument::Compact));
		setMessage("Unknown enum value (allowed values are: " + strJson + ")");
	}
}
