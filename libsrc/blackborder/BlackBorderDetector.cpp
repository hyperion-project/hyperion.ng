#include <iostream>
#include <utils/Logger.h>

// BlackBorders includes
#include <blackborder/BlackBorderDetector.h>
#include <cmath>

using namespace hyperion;

BlackBorderDetector::BlackBorderDetector(double threshold)
	: _blackborderThreshold(calculateThreshold(threshold))
{
	// empty
}

uint8_t BlackBorderDetector::calculateThreshold(double threshold) const
{
	int rgbThreshold = int(std::ceil(threshold * 255));
	if (rgbThreshold < 0)
		rgbThreshold = 0;
	else if (rgbThreshold > 255)
		rgbThreshold = 255;

	uint8_t blackborderThreshold = uint8_t(rgbThreshold);

	return blackborderThreshold;
}
