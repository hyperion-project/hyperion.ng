#pragma once
#include <QString>

#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>

namespace RGBW {

	enum WhiteAlgorithm {
		INVALID,
		SUBTRACT_MINIMUM,
		SUB_MIN_WARM_ADJUST,
		SUB_MIN_COOL_ADJUST,
		WHITE_OFF
	};

	WhiteAlgorithm stringToWhiteAlgorithm(QString str);
	void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, const WhiteAlgorithm algorithm);

}
