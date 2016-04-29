#pragma once

// STL includes
#include <string>

// Utils includes
#include <utils/RgbChannelAdjustment.h>

class ColorAdjustment
{
public:

	/// Unique identifier for this color transform
	std::string _id;

	/// The RED-Channel (RGB) adjustment
	RgbChannelAdjustment _rgbRedAdjustment;
	/// The GREEN-Channel (RGB) transform
	RgbChannelAdjustment _rgbGreenAdjustment;
	/// The BLUE-Channel (RGB) transform
	RgbChannelAdjustment _rgbBlueAdjustment;
};
