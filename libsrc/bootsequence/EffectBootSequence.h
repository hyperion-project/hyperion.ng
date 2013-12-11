#pragma once

// Bootsequence include
#include <bootsequence/BootSequence.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

///
/// The EffectBootSequence runs a script using the effect engine at startup
///
class EffectBootSequence : public BootSequence
{
public:
	///
	/// Constructs the effect boot-sequence. The effect engine is used for executing the boot effect. The given
	/// duration is the length the effect will run.
	///
	/// @param[in] hyperion  The Hyperion instance
	/// @param[in] effect The effect definition
	/// @param[in] duration_ms  The length of the sequence [ms]
	///
	EffectBootSequence(Hyperion * hyperion, const EffectDefinition & effect, const unsigned duration_ms);
	virtual ~EffectBootSequence();

	virtual void start();

private:
	/// The Hyperion instance
	Hyperion * _hyperion;

	/// The script to execute
	const EffectDefinition _effect;

	/// Duration of the boot sequence
	const unsigned _duration_ms;
};

