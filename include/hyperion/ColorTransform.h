#pragma once

// STL includes
#include <string>

// Utils includes
#include <utils/RgbChannelTransform.h>
#include <utils/HsvTransform.h>

class ColorTransform
{
public:

	/// Unique identifier for this color transform
	std::string _id;

	/// The RED-Channel (RGB) transform
	RgbChannelTransform _rgbRedTransform;
	/// The GREEN-Channel (RGB) transform
	RgbChannelTransform _rgbGreenTransform;
	/// The BLUE-Channel (RGB) transform
	RgbChannelTransform _rgbBlueTransform;

	/// The HSV Transform for applying Saturation and Value transforms
	HsvTransform _hsvTransform;
};
