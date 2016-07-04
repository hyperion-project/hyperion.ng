#pragma once

// STL includes
#include <string>

// Utils includes
#include <utils/RgbChannelAdjustment.h>

class ColorCorrection
{
public:
	
	/// Unique identifier for this color correction
	std::string _id;

	/// The RGB correction
	RgbChannelAdjustment _rgbCorrection;
};
