#pragma once

// stl includes
#include <string>
#include <list>

// jsoncpp includes
#include <json/json.h>


/// JsonSchemaChecker is a very basic implementation of json schema.
/// The json schema definition draft can be found at
/// http://tools.ietf.org/html/draft-zyp-json-schema-03
///
/// The following keywords are supported:
/// - type
/// - required
/// - properties
/// - items
/// - enum
/// - minimum
/// - maximum
/// - addtionalProperties
/// - minItems
/// - maxItems
///
/// And the non-standard:
/// - dependencies
class JsonSchemaChecker
{
public:
	JsonSchemaChecker();
	virtual ~JsonSchemaChecker();

	///
	/// @param The schema to use
	/// @return true upon succes
	///
	bool setSchema(const Json::Value & schema);

	///
	/// @brief Validate a JSON structure
	/// @param The JSON value to check
	/// @return true when the arguments is valid according to the schema
	///
	bool validate(const Json::Value & value);

	///
	/// @return A list of error messages
	///
	const std::list<std::string> & getMessages() const;

private:
	void collectReferences(const Json::Value & schema);

	void validate(const Json::Value &value, const Json::Value & schema);

	void setMessage(const std::string & message);

	void collectDependencies(const Json::Value & value, const Json::Value &schema);

private:
	// attribute check functions
	void checkType(const Json::Value & value, const Json::Value & schema);
	void checkProperties(const Json::Value & value, const Json::Value & schema);
	void checkAdditionalProperties(const Json::Value & value, const Json::Value & schema, const Json::Value::Members & ignoredProperties);
	void checkDependencies(const Json::Value & value, const Json::Value & schemaLink);
	void checkMinimum(const Json::Value & value, const Json::Value & schema);
	void checkMaximum(const Json::Value & value, const Json::Value & schema);
	void checkItems(const Json::Value & value, const Json::Value & schema);
	void checkMinItems(const Json::Value & value, const Json::Value & schema);
	void checkMaxItems(const Json::Value & value, const Json::Value & schema);
	void checkUniqueItems(const Json::Value & value, const Json::Value & schema);
	void checkEnum(const Json::Value & value, const Json::Value & schema);

private:
	Json::Value _schema;

	std::list<std::string> _currentPath;
	std::list<std::string> _messages;
	bool _error;

	std::map<std::string, const Json::Value *> _references; // ref 2 value
};
