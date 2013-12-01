#include "EffectBootSequence.h"

EffectBootSequence::EffectBootSequence(Hyperion *hyperion, const std::string &script, const Json::Value &args, const unsigned duration_ms) :
	BootSequence(),
	_hyperion(hyperion),
	_script(script),
	_args(args),
	_duration_ms(duration_ms)
{
}

EffectBootSequence::~EffectBootSequence()
{
}

void EffectBootSequence::start()
{
	_hyperion->setEffectScript(_script, _args, 0, _duration_ms);
}
