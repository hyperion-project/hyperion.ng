#pragma once

// STL incldues
#include <vector>

// Utility includes
#include <utils/ColorRgb.h>

///
/// Interface (pure virtual base class) for LedDevices.
///
class LedDevice
{
public:

	///
	/// Empty virtual destructor for pure virtual base class
	///
	virtual ~LedDevice()
	{
		// empty
	}

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues) = 0;

	/// Switch the leds off
	virtual int switchOff() = 0;
};
