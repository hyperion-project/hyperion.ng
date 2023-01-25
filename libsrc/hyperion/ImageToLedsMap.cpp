#include <hyperion/ImageToLedsMap.h>

using namespace hyperion;

ImageToLedsMap::ImageToLedsMap(
		int width,
		int height,
		int horizontalBorder,
		int verticalBorder,
		const std::vector<Led>& leds)
	: _width(width)
	, _height(height)
	, _horizontalBorder(horizontalBorder)
	, _verticalBorder(verticalBorder)
	, _colorsMap()
{
	// Sanity check of the size of the borders (and width and height)
	Q_ASSERT(_width  > 2*_verticalBorder);
	Q_ASSERT(_height > 2*_horizontalBorder);
	Q_ASSERT(_width  < 10000);
	Q_ASSERT(_height < 10000);

	// Reserve enough space in the map for the leds
	_colorsMap.reserve(leds.size());

	const int xOffset      = _verticalBorder;
	const int actualWidth  = _width  - 2 * _verticalBorder;
	const int yOffset      = _horizontalBorder;
	const int actualHeight = _height - 2 * _horizontalBorder;

	size_t	totalCount = 0;
	size_t	totalCapacity = 0;

	for (const Led& led : leds)
	{
		// skip leds without area
		if ((led.maxX_frac-led.minX_frac) < 1e-6 || (led.maxY_frac-led.minY_frac) < 1e-6)
		{
			_colorsMap.emplace_back();
			continue;
		}

		// Compute the index boundaries for this led
		int minX_idx = xOffset + int32_t(qRound(actualWidth  * led.minX_frac));
		int maxX_idx = xOffset + int32_t(qRound(actualWidth  * led.maxX_frac));
		int minY_idx = yOffset + int32_t(qRound(actualHeight * led.minY_frac));
		int maxY_idx = yOffset + int32_t(qRound(actualHeight * led.maxY_frac));

		// make sure that the area is at least a single led large
		minX_idx = qMin(minX_idx, xOffset + actualWidth - 1);
		if (minX_idx == maxX_idx)
		{
			maxX_idx++;
		}
		minY_idx = qMin(minY_idx, yOffset + actualHeight - 1);
		if (minY_idx == maxY_idx)
		{
			maxY_idx++;
		}

		// Add all the indices in the above defined rectangle to the indices for this led
		const int maxYLedCount = qMin(maxY_idx, yOffset+actualHeight);
		const int maxXLedCount = qMin(maxX_idx, xOffset+actualWidth);

		const int realYLedCount = qAbs(maxYLedCount - minY_idx);
		const int realXLedCount = qAbs(maxXLedCount - minX_idx);

		size_t totalSize = realYLedCount* realXLedCount;

		std::vector<int> ledColors;
		ledColors.reserve(totalSize);

		for (int y = minY_idx; y < maxYLedCount; ++y)
		{
			for (int x = minX_idx; x < maxXLedCount; ++x)
			{
				ledColors.push_back( y * width + x);
			}
		}

		// Add the constructed vector to the map
		_colorsMap.push_back(ledColors);

		totalCount += ledColors.size();
		totalCapacity += ledColors.capacity();

	}
	Debug(Logger::getInstance("HYPERION"), "Total index number is: %d (memory: %d). image size: %d x %d, LED areas: %d",
		totalCount, totalCapacity, width, height, leds.size());

}

int ImageToLedsMap::width() const
{
	return _width;
}

int ImageToLedsMap::height() const
{
	return _height;
}
