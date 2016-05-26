// stdlib includes
#include <cassert>
#include <iterator>
#include <sstream>
#include <algorithm>

// Utils-Jsonschema includes
#include <utils/jsonschema/JsonSchemaChecker.h>

JsonSchemaChecker::JsonSchemaChecker()
{
	// empty
}

JsonSchemaChecker::~JsonSchemaChecker()
{
	// empty
}

bool JsonSchemaChecker::setSchema(const Json::Value & schema)
{
	_schema = schema;

	// TODO: check the schema

	return true;
}

bool JsonSchemaChecker::validate(const Json::Value & value)
{
	// initialize state
	_error = false;
	_messages.clear();
	_currentPath.clear();
	_currentPath.push_back("[root]");
	_references.clear();

	// collect dependencies
	collectDependencies(value, _schema);

	// validate
	validate(value, _schema);

	return !_error;
}

void JsonSchemaChecker::collectDependencies(const Json::Value & value, const Json::Value &schema)
{
	assert (schema.isObject());

	// check if id is present
	if (schema.isMember("id"))
	{
		// strore reference
		assert (schema["id"].isString());
		std::ostringstream ref;
		ref << "$(" << schema["id"].asString() << ")";
		_references[ref.str()] = &value;
	}

	// check the current json value
	if (schema.isMember("properties"))
	{
		const Json::Value & properties = schema["properties"];
		assert(properties.isObject());

		for (Json::Value::const_iterator j = properties.begin(); j != properties.end(); ++j)
		{
			std::string property = j.memberName();
			if (value.isMember(property))
			{
				collectDependencies(value[property], properties[property]);
			}
		}
	}
}

void JsonSchemaChecker::validate(const Json::Value & value, const Json::Value &schema)
{
	assert (schema.isObject());

	// check the current json value
	for (Json::Value::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		std::string attribute = i.memberName();
		const Json::Value & attributeValue = *i;

		if (attribute == "type")
			checkType(value, attributeValue);
		else if (attribute == "properties")
			checkProperties(value, attributeValue);
		else if (attribute == "additionalProperties")
		{
			// ignore the properties which are handled by the properties attribute (if present)
			Json::Value::Members ignoredProperties;
			if (schema.isMember("properties")) {
				const Json::Value & props = schema["properties"];
				ignoredProperties = props.getMemberNames();
			}

			checkAdditionalProperties(value, attributeValue, ignoredProperties);
		}
		else if (attribute == "dependencies")
			checkDependencies(value, attributeValue);
		else if (attribute == "minimum")
			checkMinimum(value, attributeValue);
		else if (attribute == "maximum")
			checkMaximum(value, attributeValue);
		else if (attribute == "items")
			checkItems(value, attributeValue);
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
		else
		{
			// no check function defined for this attribute
			setMessage(std::string("No check function defined for attribute ") + attribute);
			continue;
		}
	}
}

void JsonSchemaChecker::setMessage(const std::string & message)
{
	std::ostringstream oss;
	std::copy(_currentPath.begin(), _currentPath.end(), std::ostream_iterator<std::string>(oss, ""));
	oss << ": " << message;
	_messages.push_back(oss.str());
}

const std::list<std::string> & JsonSchemaChecker::getMessages() const
{
	return _messages;
}

void JsonSchemaChecker::checkType(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isString());

	std::string type = schema.asString();
	bool wrongType = false;
	if (type == "string")
		wrongType = !value.isString();
	else if (type == "number")
		wrongType = !value.isNumeric();
	else if (type == "integer")
		wrongType = !value.isIntegral();
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

void JsonSchemaChecker::checkProperties(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isObject());

	if (!value.isObject())
	{
		_error = true;
		setMessage("properies attribute is only valid for objects");
		return;
	}

	for (Json::Value::const_iterator i = schema.begin(); i != schema.end(); ++i)
	{
		std::string property = i.memberName();
		const Json::Value & propertyValue = *i;

		assert(propertyValue.isObject());

		_currentPath.push_back(std::string(".") + property);
		if (value.isMember(property))
		{
			validate(value[property], propertyValue);
		}
		else if (propertyValue.get("required", false).asBool())
		{
			_error = true;
			setMessage("missing member");
		}
		_currentPath.pop_back();
	}
}

void JsonSchemaChecker::checkAdditionalProperties(const Json::Value & value, const Json::Value & schema, const Json::Value::Members & ignoredProperties)
{
	if (!value.isObject())
	{
		_error = true;
		setMessage("additional properies attribute is only valid for objects");
		return;
	}

	for (Json::Value::const_iterator i = value.begin(); i != value.end(); ++i)
	{
		std::string property = i.memberName();
		if (std::find(ignoredProperties.begin(), ignoredProperties.end(), property) == ignoredProperties.end())
		{
			// property has no property definition. check against the definition for additional properties
			_currentPath.push_back(std::string(".") + property);
			if (schema.isBool())
			{
				if (schema.asBool() == false)
				{
					_error = true;
					setMessage("no schema definition");
				}
			}
			else
			{
				validate(value[property], schema);
			}
			_currentPath.pop_back();
		}
	}
}

void JsonSchemaChecker::checkDependencies(const Json::Value & value, const Json::Value & schemaLink)
{
	if (!value.isObject())
	{
		_error = true;
		setMessage("dependencies attribute is only valid for objects");
		return;
	}

	assert(schemaLink.isString());
	std::map<std::string, const Json::Value *>::iterator iter = _references.find(schemaLink.asString());
	if (iter == _references.end())
	{
		_error = true;
		std::ostringstream oss;
		oss << "reference " << schemaLink.asString() << " could not be resolved";
		setMessage(oss.str());
		return;
	}
	const Json::Value & schema = *(iter->second);

	std::list<std::string> requiredProperties;
	if (schema.isString())
	{
		requiredProperties.push_back(schema.asString());
	}
	else if (schema.isArray())
	{
		for (Json::UInt i = 0; i < schema.size(); ++i)
		{
			assert(schema[i].isString());
			requiredProperties.push_back(schema[i].asString());
		}
	}
	else
	{
		_error = true;
		std::ostringstream oss;
		oss << "Exepected reference " << schemaLink.asString() << " to resolve to a string or array";
		setMessage(oss.str());
		return;
	}

	for (std::list<std::string>::const_iterator i = requiredProperties.begin(); i != requiredProperties.end(); ++i)
	{
		if (!value.isMember(*i))
		{
			_error = true;
			std::ostringstream oss;
			oss << "missing member " << *i;
			setMessage(oss.str());
		}
	}
}

void JsonSchemaChecker::checkMinimum(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isNumeric());

	if (!value.isNumeric())
	{
		// only for numeric
		_error = true;
		setMessage("minimum check only for numeric fields");
		return;
	}

	if (value.asDouble() < schema.asDouble())
	{
		_error = true;
		std::ostringstream oss;
		oss << "value is too small (minimum=" << schema.asDouble() << ")";
		setMessage(oss.str());
	}
}

void JsonSchemaChecker::checkMaximum(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isNumeric());

	if (!value.isNumeric())
	{
		// only for numeric
		_error = true;
		setMessage("maximum check only for numeric fields");
		return;
	}

	if (value.asDouble() > schema.asDouble())
	{
		_error = true;
		std::ostringstream oss;
		oss << "value is too large (maximum=" << schema.asDouble() << ")";
		setMessage(oss.str());
	}
}

void JsonSchemaChecker::checkItems(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isObject());

	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("items only valid for arrays");
		return;
	}

	for(Json::ArrayIndex i = 0; i < value.size(); ++i)
	{
		// validate each item
		std::ostringstream oss;
		oss << "[" << i << "]";
		_currentPath.push_back(oss.str());
		validate(value[i], schema);
		_currentPath.pop_back();
	}
}

void JsonSchemaChecker::checkMinItems(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isIntegral());

	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("minItems only valid for arrays");
		return;
	}

	int minimum = schema.asInt();

	if (static_cast<int>(value.size()) < minimum)
	{
		_error = true;
		std::ostringstream oss;
		oss << "array is too small (minimum=" << minimum << ")";
		setMessage(oss.str());
	}
}

void JsonSchemaChecker::checkMaxItems(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isIntegral());

	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("maxItems only valid for arrays");
		return;
	}

	int maximum = schema.asInt();

	if (static_cast<int>(value.size()) > maximum)
	{
		_error = true;
		std::ostringstream oss;
		oss << "array is too large (maximum=" << maximum << ")";
		setMessage(oss.str());
	}
}

void JsonSchemaChecker::checkUniqueItems(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isBool());

	if (!value.isArray())
	{
		// only for arrays
		_error = true;
		setMessage("uniqueItems only valid for arrays");
		return;
	}

	if (schema.asBool() == true)
	{
		// make sure no two items are identical

		for(Json::UInt i = 0; i < value.size(); ++i)
		{
			for (Json::UInt j = i+1; j < value.size(); ++j)
			{
				if (value[i] == value[j])
				{
					// found a value twice
					_error = true;
					setMessage("array must have unique values");
				}
			}
		}
	}
}

void JsonSchemaChecker::checkEnum(const Json::Value & value, const Json::Value & schema)
{
	assert(schema.isArray());

	for(Json::ArrayIndex i = 0; i < schema.size(); ++i)
	{
		if (schema[i] == value)
		{
			// found enum value. done.
			return;
		}
	}

	// nothing found
	_error = true;
	std::ostringstream oss;
	oss << "Unknown enum value (allowed values are: ";
	std::string values = Json::FastWriter().write(schema);
	oss << values.substr(0, values.size()-1); // The writer append a new line which we don't want
	oss << ")";
	setMessage(oss.str());
}
