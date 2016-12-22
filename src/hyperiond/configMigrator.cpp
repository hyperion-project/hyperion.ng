#include "configMigrator.h"


ConfigMigrator::ConfigMigrator()
	: _log(Logger::getInstance("ConfigMigrator"))
{
}

ConfigMigrator::~ConfigMigrator()
{
}

bool ConfigMigrator::migrate(QString configFile, int fromVersion,int toVersion)
{
	Debug(_log, "migrate config %s from version %d to %d.", configFile.toLocal8Bit().constData(), fromVersion, toVersion);
	throw std::runtime_error("ERROR: config migration not implemented");
	return true;
}
 
