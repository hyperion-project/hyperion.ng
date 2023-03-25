#pragma once
#include <QString>

#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>

namespace RGBW {

	enum class WhiteAlgorithm {
		INVALID,
		SUBTRACT_MINIMUM,
		SUB_MIN_WARM_ADJUST,
		SUB_MIN_COOL_ADJUST,
		WHITE_OFF,
        HYPERSERIAL_COLD_WHITE,
        HYPERSERIAL_NEUTRAL_WHITE,
        WLED_AUTO,
        WLED_AUTO_MAX,
        WLED_AUTO_ACCURATE
	};

	WhiteAlgorithm stringToWhiteAlgorithm(const QString& str);
	void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, WhiteAlgorithm algorithm);
}
