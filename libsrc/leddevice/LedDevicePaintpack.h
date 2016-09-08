#pragma once

// STL includes
#include <vector>

// Hyperion includes
#include "ProviderHID.h"

///
/// LedDevice implementation for a paintpack device ()
///
class LedDevicePaintpack : public ProviderHID
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDevicePaintpack(const Json::Value &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues);

	///
	/// Switch the leds off
	///
	/// @return Zero on success else negative
	///
	virtual int switchOff();
};
