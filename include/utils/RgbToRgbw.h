#pragma once

#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>

namespace RGBW {

	enum WhiteAlgorithm { INVALID, SUBTRACT_MINIMUM, SUB_MIN_WARM_ADJUST, WHITE_OFF };
	
	WhiteAlgorithm stringToWhiteAlgorithm(std::string str);
	void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, const WhiteAlgorithm algorithm);

};
