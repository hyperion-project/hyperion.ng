#include "configMigrator.h"


ConfigMigrator::ConfigMigrator()
{
}

ConfigMigrator::~ConfigMigrator()
{
}

bool ConfigMigrator::migrate(QString configFile, int fromVersion,int toVersion)
{
	Debug(_log, "migrate config %s from version %d to %d.", configFile.toLocal8Bit().constData(), fromVersion, toVersion);
	
	for (int v=fromVersion; v<toVersion; v++)
	{
		switch(v)
		{
			case 1: migrateFrom1(); break;

			default:
				throw std::runtime_error("ERROR: config migration - unknown version");
		}
	}
	
	return true;
}

void ConfigMigrator::migrateFrom1()
{
	throw std::runtime_error("ERROR: config migration not implemented");
}

