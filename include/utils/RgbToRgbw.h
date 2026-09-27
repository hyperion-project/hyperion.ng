#pragma once
#include <QString>

#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/KelvinToRgb.h>

namespace RGBW {

	enum class WhiteAlgorithm {
		INVALID,
		SUBTRACT_MINIMUM,
		SUB_MIN_WARM_ADJUST,
		SUB_MIN_COOL_ADJUST,
		SUB_KTEMP_WHITE,
		WHITE_OFF,
        COLD_WHITE,
        NEUTRAL_WHITE,
        AUTO,
        AUTO_MAX,
        AUTO_ACCURATE
	};

	WhiteAlgorithm stringToWhiteAlgorithm(const QString& str);
	void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, WhiteAlgorithm algorithm, int whiteTemp = ColorTemperature::DEFAULT);
}
