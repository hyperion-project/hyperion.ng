#pragma once

// stl include
#include <string>

// json include
#include <json/value.h>

struct EffectDefinition
{
	std::string name;
	std::string script;
	Json::Value args;
};
