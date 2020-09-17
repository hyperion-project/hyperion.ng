#pragma once

// qt includes
#include <QFile>
#include <QString>
#include <QByteArray>

// util includes
#include "Logger.h"

namespace FileUtils {

	QString getBaseName(const QString& sourceFile);
	QString getDirName(const QString& sourceFile);

	///
	/// @brief remove directory recursive given by path
	/// @param[in]   path     Path to directory
	bool removeDir(const QString& path, Logger* log);

	///
	/// @brief check if the file exists
	/// @param[in]   path     The file path to check
	/// @param[in]   log      The logger of the caller to print errors
	/// @param[in]   ignError Ignore errors during file read (no log output)
	/// @return               true on success else false
	///
	bool fileExists(const QString& path, Logger* log, bool ignError=false);

	///
	/// @brief read a file given by path.
	/// @param[in]   path     The file path to read
	/// @param[out]  data     The read data o success
	/// @param[in]   log      The logger of the caller to print errors
	/// @param[in]   ignError Ignore errors during file read (no log output)
	/// @return               true on success else false
	///
	bool readFile(const QString& path, QString& data, Logger* log, bool ignError=false);

	///
	/// write a file given by path.
	/// @param[in]   path     The file path to read
	/// @param[in]   data     The data to write
	/// @param[in]   log      The logger of the caller to print errors
	/// @return               true on success else false
	///
	bool writeFile(const QString& path, const QByteArray& data, Logger* log);

	///
	/// @brief delete a file by given path
	/// @param[in]   path     The file path to delete
	/// @param[in]   log      The logger of the caller to print errors
	/// @param[in]   ignError Ignore errors during file delete (no log output)
	/// @return               true on success else false
	///
	bool removeFile(const QString& path, Logger* log, bool ignError=false);

	///
	/// @brief resolve the file error and print a message
	/// @param[in]  file     The file which caused the error
	/// @param[in]  log      The logger of the caller
	///
	void resolveFileError(const QFile& file, Logger* log);
}
