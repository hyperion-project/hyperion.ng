#pragma once

#include <utils/Logger.h>

#include <QString>

class ConfigMigrator
{

public:
	ConfigMigrator();
	~ConfigMigrator();

	bool migrate(QString configFile, int fromVersion,int toVersion);
private:
	Logger * _log;
};