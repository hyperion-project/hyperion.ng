#pragma once

#include "configMigratorBase.h"
#include <QString>

///
/// class that contains migration code
/// helper code goeas to base class
class ConfigMigrator : public ConfigMigratorBase

{

public:
	ConfigMigrator();
	~ConfigMigrator();

	bool migrate(QString configFile, int fromVersion,int toVersion);

private:
	void migrateFrom1();
};