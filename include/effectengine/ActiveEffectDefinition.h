#pragma once

// stl include
#include <string>

// json include
#include <json/value.h>

struct ActiveEffectDefinition
{
	std::string script;
	std::string name;
	int priority;
	int timeout;
	Json::Value args;
};
