
// STL includes
#include <algorithm>

// hyperion includes
#include "ImageToLedsMap.h"

using namespace hyperion;

ImageToLedsMap::ImageToLedsMap(const RgbImage& image, const std::vector<Led>& leds)
{
	mColorsMap.resize(leds.size(), std::vector<const RgbColor*>());

	auto ledColors = mColorsMap.begin();
	for (auto led = leds.begin(); ledColors != mColorsMap.end() && led != leds.end(); ++ledColors, ++led)
	{
		ledColors->clear();

		const unsigned minX_idx = unsigned(image.width()  * led->minX_frac);
		const unsigned maxX_idx = unsigned(image.width()  * led->maxX_frac);
		const unsigned minY_idx = unsigned(image.height() * led->minY_frac);
		const unsigned maxY_idx = unsigned(image.height() * led->maxY_frac);

		for (unsigned y = minY_idx; y<=maxY_idx && y<image.height(); ++y)
		{
			for (unsigned x = minX_idx; x<=maxX_idx && x<image.width(); ++x)
			{
				ledColors->push_back(&image(x,y));
			}
		}
	}
}

std::vector<RgbColor> ImageToLedsMap::getMeanLedColor()
{
	std::vector<RgbColor> colors;
	for (auto ledColors = mColorsMap.begin(); ledColors != mColorsMap.end(); ++ledColors)
	{
		const RgbColor color = findMeanColor(*ledColors);
		colors.push_back(color);
	}

	return colors;
}

void ImageToLedsMap::getMeanLedColor(std::vector<RgbColor>& ledColors)
{
	// Sanity check for the number of leds
	assert(mColorsMap.size() == ledColors.size());

	auto led = ledColors.begin();
	for (auto ledColors = mColorsMap.begin(); ledColors != mColorsMap.end(); ++ledColors, ++led)
	{
		const RgbColor color = findMeanColor(*ledColors);
		*led = color;
	}
}

RgbColor ImageToLedsMap::findMeanColor(const std::vector<const RgbColor*>& colors)
{
	uint_fast16_t cummRed   = 0;
	uint_fast16_t cummGreen = 0;
	uint_fast16_t cummBlue  = 0;
	for (const RgbColor* color : colors)
	{
		cummRed   += color->red;
		cummGreen += color->green;
		cummBlue  += color->blue;
	}

	const uint8_t avgRed   = uint8_t(cummRed/colors.size());
	const uint8_t avgGreen = uint8_t(cummGreen/colors.size());
	const uint8_t avgBlue  = uint8_t(cummBlue/colors.size());

	return {avgRed, avgGreen, avgBlue};
}
