#include "EffectBootSequence.h"

EffectBootSequence::EffectBootSequence(Hyperion *hyperion, const EffectDefinition &effect, const unsigned duration_ms) :
	BootSequence(),
	_hyperion(hyperion),
	_effect(effect),
	_duration_ms(duration_ms)
{
}

EffectBootSequence::~EffectBootSequence()
{
}

void EffectBootSequence::start()
{
	_hyperion->setEffectScript(_effect.script, _effect.args, 0, _duration_ms);
}
