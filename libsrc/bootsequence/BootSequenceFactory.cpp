// stl includes
#include <cctype>
#include <algorithm>

// Bootsequence includes
#include <bootsequence/BootSequenceFactory.h>

// Effect engine includes
#include <effectengine/EffectEngine.h>

// Local Bootsequence includes
#include "EffectBootSequence.h"

BootSequence * BootSequenceFactory::createBootSequence(Hyperion * hyperion, const Json::Value & jsonConfig)
{
	const std::string path = jsonConfig["path"].asString();
	const std::string effectFile = jsonConfig["effect"].asString();
	const unsigned duration = jsonConfig["duration_ms"].asUInt();

	EffectDefinition effect;
	if (EffectEngine::loadEffectDefinition(path, effectFile, effect))
	{
		return new EffectBootSequence(hyperion, effect, duration);
	}

	std::cerr << "Boot sequence could not be loaded" << std::endl;
	return nullptr;
}
