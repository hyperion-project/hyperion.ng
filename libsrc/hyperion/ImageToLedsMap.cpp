
// STL includes
#include <algorithm>

// hyperion includes
#include "ImageToLedsMap.h"

using namespace hyperion;

ImageToLedsMap::ImageToLedsMap(const unsigned width, const unsigned height, const std::vector<Led>& leds) :
	_width(width),
	_height(height),
	mColorsMap()
{
	// Reserve enough space in the map for the leds
	mColorsMap.reserve(leds.size());

	for (const Led& led : leds)
	{
		const unsigned minX_idx = unsigned(width  * led.minX_frac);
		const unsigned maxX_idx = unsigned(width  * led.maxX_frac);
		const unsigned minY_idx = unsigned(height * led.minY_frac);
		const unsigned maxY_idx = unsigned(height * led.maxY_frac);

		std::vector<unsigned> ledColors;
		for (unsigned y = minY_idx; y<=maxY_idx && y<height; ++y)
		{
			for (unsigned x = minX_idx; x<=maxX_idx && x<width; ++x)
			{
				ledColors.push_back(y*width + x);
			}
		}
		mColorsMap.push_back(ledColors);
	}
}

unsigned ImageToLedsMap::width() const
{
	return _width;
}

unsigned ImageToLedsMap::height() const
{
	return _height;
}

std::vector<RgbColor> ImageToLedsMap::getMeanLedColor(const RgbImage & image) const
{
	std::vector<RgbColor> colors(mColorsMap.size(), RgbColor::BLACK);
	getMeanLedColor(image, colors);
	return colors;
}

void ImageToLedsMap::getMeanLedColor(const RgbImage & image, std::vector<RgbColor> & ledColors) const
{
	// Sanity check for the number of leds
	assert(mColorsMap.size() == ledColors.size());

	auto led = ledColors.begin();
	for (auto ledColors = mColorsMap.begin(); ledColors != mColorsMap.end(); ++ledColors, ++led)
	{
		const RgbColor color = calcMeanColor(image, *ledColors);
		*led = color;
	}
}

RgbColor ImageToLedsMap::calcMeanColor(const RgbImage & image, const std::vector<unsigned> & colors) const
{
	uint_fast16_t cummRed   = 0;
	uint_fast16_t cummGreen = 0;
	uint_fast16_t cummBlue  = 0;
	for (const unsigned colorOffset : colors)
	{
		const RgbColor& color = image.memptr()[colorOffset];
		cummRed   += color.red;
		cummGreen += color.green;
		cummBlue  += color.blue;
	}

	const uint8_t avgRed   = uint8_t(cummRed/colors.size());
	const uint8_t avgGreen = uint8_t(cummGreen/colors.size());
	const uint8_t avgBlue  = uint8_t(cummBlue/colors.size());

	return {avgRed, avgGreen, avgBlue};
}
