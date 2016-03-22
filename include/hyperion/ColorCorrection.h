#pragma once

// STL includes
#include <string>

// Utils includes
#include <utils/RgbChannelCorrection.h>

class ColorCorrection
{
public:
	
	/// Unique identifier for this color correction
	std::string _id;

	/// The RGB correction
	RgbChannelCorrection _rgbCorrection;
};
