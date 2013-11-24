#include <effectengine/EffectEngine.h>

EffectEngine::EffectEngine(Hyperion * hyperion) :
	_hyperion(hyperion),
	_availableEffects()
{
	// connect the Hyperion channel clear feedback
	connect(_hyperion, SIGNAL(channelCleared(int)), this, SLOT(channelCleared(int)));
	connect(_hyperion, SIGNAL(allChannelsCleared()), this, SLOT(allChannelsCleared()));

	// read all effects
	_availableEffects["test"] = "test.py";
}

EffectEngine::~EffectEngine()
{
}

std::list<std::string> EffectEngine::getEffects() const
{
	std::list<std::string> effectNames;
	foreach (auto entry, _availableEffects) {
		effectNames.push_back(entry.first);
	}
	return effectNames;
}

int EffectEngine::runEffect(const std::string &effectName, int priority, int timeout)
{
	std::cout << "run effect " << effectName << " on channel " << priority << std::endl;
	return 0;
}

void EffectEngine::channelCleared(int priority)
{
	std::cout << "clear effect on channel " << priority << std::endl;
}

void EffectEngine::allChannelsCleared()
{
	std::cout << "clear effect on every channel" << std::endl;
}
