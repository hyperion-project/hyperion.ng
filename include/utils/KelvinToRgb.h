#ifndef KELVINTORGB_H
#define KELVINTORGB_H

#include <cmath>

#include <utils/ColorRgb.h>


// Constants
namespace {
const int TEMPERATURE_MINIMUM = 1000;
const int TEMPERATUR_MAXIMUM = 40000;
} //End of constants

static ColorRgb getRgbFromTemperature(int temperature)
{
	//Temperature input in Kelvin valid in the range 1000 K to 40000 K. White light = 6600K
	temperature = qBound(TEMPERATURE_MINIMUM, temperature, TEMPERATUR_MAXIMUM);

	// All calculations require temperature / 100, so only do the conversion once.
	temperature /= 100;

	// Compute each color in turn.
	int red, green, blue;

	// red
	if (temperature <= 66)
	{
		red = 255;
	}
	else
	{
		// Note: the R-squared value for this approximation is 0.988.
		red = static_cast<int>(329.698727446 * (pow(temperature - 60, -0.1332047592)));
	}

	// green
	if (temperature <= 66)
	{
		// Note: the R-squared value for this approximation is 0.996.
		green = static_cast<int>(99.4708025861 * log(temperature) - 161.1195681661);

	}
	else
	{
		// Note: the R-squared value for this approximation is 0.987.
		green = static_cast<int>(288.1221695283 * (pow(temperature - 60, -0.0755148492)));
	}

	// blue
	if (temperature >= 66)
	{
		blue = 255;
	}
	else if (temperature <= 19)
	{
		blue = 0;
	}
	else
	{
		// Note: the R-squared value for this approximation is 0.998.
		blue = static_cast<int>(138.5177312231 * log(temperature - 10) - 305.0447927307);
	}

	return {
		static_cast<uint8_t>(qBound(0, red, 255)),
		static_cast<uint8_t>(qBound(0, green, 255)),
		static_cast<uint8_t>(qBound(0, blue, 255)),
	};
}

#endif // KELVINTORGB_H
