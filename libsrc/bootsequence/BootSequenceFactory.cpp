// stl includes
#include <cctype>
#include <algorithm>

// Bootsequence includes
#include <bootsequence/BootSequenceFactory.h>

// Local Bootsequence includes
#include "RainbowBootSequence.h"
#include "KittBootSequence.h"

BootSequence * BootSequenceFactory::createBootSequence(Hyperion * hyperion, const Json::Value & jsonConfig)
{
	std::string type = jsonConfig["type"].asString();
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);

	if (type == "none")
	{
		return nullptr;
	}
	else if (type == "rainbow")
	{
		const unsigned duration_ms = jsonConfig["duration_ms"].asUInt();
		return new RainbowBootSequence(hyperion, duration_ms);
	}
	else if (type == "knightrider" || type == "knight rider")
	{
		const unsigned duration_ms = jsonConfig["duration_ms"].asUInt();
		return new KittBootSequence(hyperion, duration_ms);
	}

	std::cerr << "Unknown boot-sequence selected; boot-sequence disabled." << std::endl;
	return nullptr;
}


