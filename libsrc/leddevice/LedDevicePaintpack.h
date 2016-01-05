#pragma once

// STL includes
#include <vector>

// Hyperion includes
#include "LedHIDDevice.h"

///
/// LedDevice implementation for a paintpack device ()
///
class LedDevicePaintpack : public LedHIDDevice
{
public:
	/**
	 * Constructs the paintpack device
	 */
	LedDevicePaintpack(const unsigned short VendorId, const unsigned short ProductId, int delayAfterConnect_ms);

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

private:
	/// buffer for led data
	std::vector<uint8_t> _ledBuffer;
};
