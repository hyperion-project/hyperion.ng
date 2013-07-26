#pragma once

// STL incldues
#include <vector>

// Utility includes
#include <utils/RgbColor.h>

class LedDevice
{
public:

	/**
	 * Empty virtual destructor for pure virtual base class
	 */
	virtual ~LedDevice()
	{
		// empty
	}

	/**
	 * Writes the RGB-Color values to the leds.
	 *
	 * @param[in] ledValues  The RGB-color per led
	 */
	virtual int write(const std::vector<RgbColor>& ledValues) = 0;
};
