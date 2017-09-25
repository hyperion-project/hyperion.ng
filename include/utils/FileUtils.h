#pragma once

#include <QString>
#include "Logger.h"

namespace FileUtils {

QString getBaseName( QString sourceFile);
QString getDirName( QString sourceFile);

///
/// check if the file exists
/// param[in]   path     The file path to check
/// param[in]   log      The logger of the caller to print errors
/// param[in]   ignError Ignore errors during file read (no log output)
/// return               true on success else false
///
bool fileExists(const QString& path, Logger* log, bool ignError=false);

///
/// read a file given by path.
/// param[in]   path     The file path to read
/// param[out]  data     The read data o success
/// param[in]   log      The logger of the caller to print errors
/// param[in]   ignError Ignore errors during file read (no log output)
/// return               true on success else false
///
bool readFile(const QString& path, QString& data, Logger* log, bool ignError=false);
};
