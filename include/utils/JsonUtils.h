#pragma once

#include "FileUtils.h"
#include <QTextStream>
#include <QJsonObject>
#include <QJsonParseError>

namespace JsonUtils{
	///
	/// read a json file and get the parsed result on success
	/// param[in]  path     The file path to read
	/// param[out] obj      Retuns the parsed QJsonObject
	/// param[in]  log      The logger of the caller to print errors
	/// param[in]  ignError Ignore errors during file read (no log output)
	/// return              true on success else false
	///
	bool readFile(const QString& path, QJsonObject& obj, Logger* log, bool ignError=false);

	///
	/// parse a json QString and get the result on success
	/// param[in]   path The file path/name just used for log messages
	/// param[in]   data Data to parse
	/// param[out]  obj  Retuns the parsed QJsonObject
	/// param[in]   log  The logger of the caller to print errors
	/// return           true on success else false
	///
	bool parseJson(const QString& path, QString& data, QJsonObject& obj, Logger* log);

};
