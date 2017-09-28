#pragma once

#include <utils/FileUtils.h>

#include <QJsonObject>
#include <utils/Logger.h>

namespace JsonUtils{
	///
	/// @brief read a json file and get the parsed result on success
	/// @param[in]  path     The file path to read
	/// @param[out] obj      Returns the parsed QJsonObject
	/// @param[in]  log      The logger of the caller to print errors
	/// @param[in]  ignError Ignore errors during file read (no log output)
	/// @return              true on success else false
	///
	bool readFile(const QString& path, QJsonObject& obj, Logger* log, bool ignError=false);

	///
	/// @brief read a schema file and resolve $refs
	/// @param[in]  path     The file path to read
	/// @param[out] obj      Returns the parsed QJsonObject
	/// @param[in]  log      The logger of the caller to print errors
	/// @return              true on success else false
	///
	bool readSchema(const QString& path, QJsonObject& obj, Logger* log);

	///
	/// @brief parse a json QString and get the result on success
	/// @param[in]  path     The file path/name just used for log messages
	/// @param[in]  data     Data to parse
	/// @param[out] obj      Retuns the parsed QJsonObject
	/// @param[in]  log      The logger of the caller to print errors
	/// @return              true on success else false
	///
	bool parse(const QString& path, const QString& data, QJsonObject& obj, Logger* log);

	///
	/// @brief Validate json data against a schema
	/// @param[in]   file     The path/name of json file just used for log messages
	/// @param[in]   json     The json data
	/// @param[in]   schemaP  The schema path
	/// @param[in]   log      The logger of the caller to print errors
	/// @return               true on success else false
	///
	bool validate(const QString& file, const QJsonObject& json, const QString& schemaPath, Logger* log);

	///
	/// @brief Write json data to file
	/// @param[in]   filenameThe file path to write
	/// @param[in]   json    The json data to write
	/// @param[in]   log     The logger of the caller to print errors
	/// @return              true on success else false
	///
	bool write(const QString& filename, const QJsonObject& json, Logger* log);

	///
	/// @brief resolve schema $ref attribute
	/// @param[in]   schema  the schema to iterate
	/// @param[out]  obj     the resolved object
	/// @param[in]   log     The logger of the caller to print errors
	/// @return              true on success else false
	///
	bool resolveRefs(const QJsonObject& schema, QJsonObject& obj, Logger* log);
};
