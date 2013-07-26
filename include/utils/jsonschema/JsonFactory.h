#pragma once

#include <string>
#include <istream>
#include <fstream>
#include <stdexcept>
#include <sstream>

// JSON-Schema includes
#include <utils/jsonschema/JsonSchemaChecker.h>

class JsonFactory
{
public:

	static int load(const std::string& schema, const std::istream& config, Json::Value json);

	static int load(const std::string& schema, const std::string& config, Json::Value& json)
	{
		// Load the schema and the config trees
		Json::Value schemaTree = readJson(schema);
		Json::Value configTree = readJson(config);

		// create the validator
		JsonSchemaChecker schemaChecker;
		schemaChecker.setSchema(schemaTree);

		bool valid = schemaChecker.validate(configTree);
		for (const std::string& message : schemaChecker.getMessages())
		{
			std::cout << message << std::endl;
		}
		if (!valid)
		{
			std::cerr << "Validation failed for configuration file: " << config.c_str() << std::endl;
			return -3;
		}

		json = configTree;
		return 0;
	}

	static Json::Value readJson(const std::string& filename)
	{
		// Open the file input stream
		std::ifstream ifs(filename.c_str());
		return readJson(ifs);
	}

	static Json::Value readJson(std::istream& stream)
	{
		// will contains the root value after parsing.
		Json::Value jsonTree;

		Json::Reader reader;
		if (! reader.parse(stream, jsonTree, false))
		{
			// report to the user the failure and their locations in the document.
			std::stringstream sstream;
			sstream << "Failed to parse configuration: " << reader.getFormattedErrorMessages().c_str();

			throw std::runtime_error(sstream.str());
		}
		return jsonTree;
	}
};
