#pragma once

#include <utils/Logger.h>

#include <QString>

class ConfigMigratorBase
{

public:
	ConfigMigratorBase();
	~ConfigMigratorBase();

protected:
	Logger * _log;
};