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
		const std::vector<Led>& leds)
	: _width(width)
	, _height(height)
	, _horizontalBorder(horizontalBorder)
	, _verticalBorder(verticalBorder)
	, _colorsMap()
{
	// Sanity check of the size of the borders (and width and height)
	assert(_width  > 2*_verticalBorder);
	assert(_height > 2*_horizontalBorder);

	// Reserve enough space in the map for the leds
	_colorsMap.reserve(leds.size());

	const unsigned xOffset      = _verticalBorder;
	const unsigned actualWidth  = _width  - 2 * _verticalBorder;
	const unsigned yOffset      = _horizontalBorder;
	const unsigned actualHeight = _height - 2 * _horizontalBorder;

	for (const Led& led : leds)
	{
		// skip leds without area
		if ((led.maxX_frac-led.minX_frac) < 1e-6 || (led.maxY_frac-led.minY_frac) < 1e-6)
		{
			_colorsMap.emplace_back();
			continue;
		}

		// Compute the index boundaries for this led
		unsigned minX_idx = xOffset + unsigned(std::round(actualWidth  * led.minX_frac));
		unsigned maxX_idx = xOffset + unsigned(std::round(actualWidth  * led.maxX_frac));
		unsigned minY_idx = yOffset + unsigned(std::round(actualHeight * led.minY_frac));
		unsigned maxY_idx = yOffset + unsigned(std::round(actualHeight * led.maxY_frac));

		// make sure that the area is at least a single led large
		minX_idx = std::min(minX_idx, xOffset + actualWidth - 1);
		if (minX_idx == maxX_idx)
		{
			maxX_idx = minX_idx + 1;
		}
		minY_idx = std::min(minY_idx, yOffset + actualHeight - 1);
		if (minY_idx == maxY_idx)
		{
			maxY_idx = minY_idx + 1;
		}

		// Add all the indices in the above defined rectangle to the indices for this led
		std::vector<unsigned> ledColors;
		for (unsigned y = minY_idx; y<maxY_idx && y<(yOffset+actualHeight); ++y)
		{
			for (unsigned x = minX_idx; x<maxX_idx && x<(xOffset+actualWidth); ++x)
			{
				ledColors.push_back(y*width + x);
			}
		}

		// Add the constructed vector to the map
		_colorsMap.push_back(ledColors);
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
