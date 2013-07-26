
#pragma once

// hyperion-utils includes
#include <utils/RgbImage.h>

// hyperion includes
#include <hyperion/LedString.h>

class ImageToLedsMap
{
public:

	ImageToLedsMap();

	void createMapping(const RgbImage& image, const std::vector<Led>& leds);

	std::vector<RgbColor> getMeanLedColor();

	RgbColor findMeanColor(const std::vector<const RgbColor*>& colors);

	std::vector<RgbColor> getMedianLedColor();

	RgbColor findMedianColor(std::vector<const RgbColor*>& colors);
private:
	std::vector<std::vector<const RgbColor*> > mColorsMap;
};
