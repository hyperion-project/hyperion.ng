
// STL includes
#include <algorithm>

// hyperion includes
#include "ImageToLedsMap.h"

using namespace hyperion;

ImageToLedsMap::ImageToLedsMap(
		const unsigned width,
		const unsigned height,
		const unsigned horizontalBorder,
		const unsigned verticalBorder,
		const std::vector<Led>& leds) :
	_width(width),
	_height(height),
	mColorsMap()
{
	// Sanity check of the size of the borders (and width and height)
	assert(width  > 2*verticalBorder);
	assert(height > 2*horizontalBorder);

	// Reserve enough space in the map for the leds
	mColorsMap.reserve(leds.size());

	const unsigned xOffset = verticalBorder;
	const unsigned actualWidth  = width  - 2 * verticalBorder;
	const unsigned yOffset = horizontalBorder;
	const unsigned actualHeight = height - 2 * horizontalBorder;

	for (const Led& led : leds)
	{
		// Compute the index boundaries for this led
		const unsigned minX_idx = xOffset + unsigned(std::round((actualWidth-1)  * led.minX_frac));
		const unsigned maxX_idx = xOffset + unsigned(std::round((actualWidth-1)  * led.maxX_frac));
		const unsigned minY_idx = yOffset + unsigned(std::round((actualHeight-1) * led.minY_frac));
		const unsigned maxY_idx = yOffset + unsigned(std::round((actualHeight-1) * led.maxY_frac));

		// Add all the indices in the above defined rectangle to the indices for this led
		std::vector<unsigned> ledColors;
		for (unsigned y = minY_idx; y<=maxY_idx && y<height; ++y)
		{
			for (unsigned x = minX_idx; x<=maxX_idx && x<width; ++x)
			{
				ledColors.push_back(y*width + x);
			}
		}

		// Add the constructed vector to the map
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

	// Iterate each led and compute the mean
	auto led = ledColors.begin();
	for (auto ledColors = mColorsMap.begin(); ledColors != mColorsMap.end(); ++ledColors, ++led)
	{
		const RgbColor color = calcMeanColor(image, *ledColors);
		*led = color;
	}
}

RgbColor ImageToLedsMap::calcMeanColor(const RgbImage & image, const std::vector<unsigned> & colors) const
{
	if (colors.size() == 0)
	{
		return RgbColor::BLACK;
	}

	// Accumulate the sum of each seperate color channel
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

	// Compute the average of each color channel
	const uint8_t avgRed   = uint8_t(cummRed/colors.size());
	const uint8_t avgGreen = uint8_t(cummGreen/colors.size());
	const uint8_t avgBlue  = uint8_t(cummBlue/colors.size());

	// Return the computed color
	return {avgRed, avgGreen, avgBlue};
}
