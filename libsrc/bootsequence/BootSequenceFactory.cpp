// stl includes
#include <cctype>
#include <algorithm>

// Bootsequence includes
#include <bootsequence/BootSequenceFactory.h>

// Local Bootsequence includes
#include "EffectBootSequence.h"

BootSequence * BootSequenceFactory::createBootSequence(Hyperion * hyperion, const Json::Value & jsonConfig)
{
	const std::string script = jsonConfig["script"].asString();
	const Json::Value args = jsonConfig.get("args", Json::Value(Json::objectValue));
	const unsigned duration = jsonConfig["duration_ms"].asUInt();
	return new EffectBootSequence(hyperion, script, args, duration);
}
