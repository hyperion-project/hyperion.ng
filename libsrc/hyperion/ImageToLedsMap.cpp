
// STL includes
#include <algorithm>
#include <cmath>
#include <cassert>

// hyperion includes
#include <hyperion/ImageToLedsMap.h>

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
