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
	/// @param[in] duration_ms  The length of the sequence [ms]
	///
	EffectBootSequence(Hyperion * hyperion, const std::string & script, const Json::Value & args, const unsigned duration_ms);
	virtual ~EffectBootSequence();

	virtual void start();

private:
	/// The Hyperion instance
	Hyperion * _hyperion;

	/// The script to execute
	const std::string _script;

	/// The arguments of the script
	const Json::Value _args;

	/// Duration of the boot sequence
	const unsigned _duration_ms;
};

