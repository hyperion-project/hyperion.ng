#ifndef KELVINTORGB_H
#define KELVINTORGB_H

#include <cmath>

#include <utils/ColorRgb.h>

// Constants
namespace ColorTemperature {
	constexpr int MINIMUM {1000};
	constexpr int MAXIMUM {40000};
	constexpr int DEFAULT {6600};
}

constexpr int RGB_MAX = UINT8_MAX;
//End of constants

inline ColorRgb getRgbFromTemperature(int temperature)
{
	//Temperature input in Kelvin valid in the range 1000 K to 40000 K. White light = 6600K
	temperature = qBound(ColorTemperature::MINIMUM, temperature, ColorTemperature::MAXIMUM);

	// All calculations require temperature / 100, so only do the conversion once.
	temperature /= 100;

	// Compute each color in turn.
	int red;
	int green;
	int blue;

	// red
	if (temperature <= 66)
	{
		red = RGB_MAX;
	}
	else
	{
		// Note: the R-squared value for this approximation is 0.988.
		red = static_cast<int>(329.698727446 * (qPow(temperature - 60, -0.1332047592)));
	}

	// green
	if (temperature <= 66)
	{
		// Note: the R-squared value for this approximation is 0.996.
		green = static_cast<int>(99.4708025861 * qLn(temperature) - 161.1195681661);

	}
	else
	{
		// Note: the R-squared value for this approximation is 0.987.
		green = static_cast<int>(288.1221695283 * (qPow(temperature - 60, -0.0755148492)));
	}

	// blue
	if (temperature >= 66)
	{
		blue = RGB_MAX;
	}
	else if (temperature <= 19)
	{
		blue = 0;
	}
	else
	{
		// Note: the R-squared value for this approximation is 0.998.
		blue = static_cast<int>(138.5177312231 * qLn(temperature - 10) - 305.0447927307);
	}

	return {
		static_cast<uint8_t>(qBound(0,   red, RGB_MAX)),
		static_cast<uint8_t>(qBound(0, green, RGB_MAX)),
		static_cast<uint8_t>(qBound(0,  blue, RGB_MAX)),
	};
}

#endif // KELVINTORGB_H
