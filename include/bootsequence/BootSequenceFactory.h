#pragma once

// Jsoncpp includes
#include <json/json.h>

// Bootsequence includes
#include <bootsequence/BootSequence.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

///
/// Factory for settings based construction of a boot-sequence
///
class BootSequenceFactory
{
public:

	///
	/// Creates a BootSequence using the given configuration (and Hyperion connection). Ownship of
	/// the returned instance is transferred
	///
	/// @param[in] hyperion  The Hyperion controlling the leds
	/// @param[in] jsonConfig The boot-sequence configuration
	///
	/// @return The bootsequence (ownership is transferred to the caller
	///
	static BootSequence * createBootSequence(Hyperion * hyperion, const Json::Value & jsonConfig);
};
