#include <hyperion/ImageToLedsMap.h>

using namespace hyperion;

ImageToLedsMap::ImageToLedsMap(
		Logger* log,
		int width,
		int height,
		int horizontalBorder,
		int verticalBorder,
		const std::vector<Led>& leds,
		int reducedPixelSetFactor,
		int accuracyLevel)
	: _log(log)
	, _width(width)
	, _height(height)
	, _horizontalBorder(horizontalBorder)
	, _verticalBorder(verticalBorder)
	, _nextPixelCount(reducedPixelSetFactor)
	, _clusterCount()
	, _colorsMap()
{
	_nextPixelCount = reducedPixelSetFactor + 1;
	setAccuracyLevel(accuracyLevel);

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
	int     ledCounter = 0;

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

		bool skipPixelProcessing {false};
		if (_nextPixelCount > 1)
		{
			skipPixelProcessing = true;
		}

		size_t totalSize = static_cast<size_t>(realYLedCount * realXLedCount);

		if (!skipPixelProcessing && totalSize > 1600)
		{
			skipPixelProcessing = true;
			_nextPixelCount = 2;
			Warning(_log, "Mapping LED/light [%d]. The current mapping area contains %d pixels which is huge. Therefore every %d pixels will be skipped. You can enable reduced processing to hide that warning.", ledCounter, totalSize, _nextPixelCount);
		}

		std::vector<int> ledColors;
		ledColors.reserve(totalSize);

		for (int y = minY_idx; y < maxYLedCount; y += _nextPixelCount)
		{
			for (int x = minX_idx; x < maxXLedCount; x += _nextPixelCount)
			{
				ledColors.push_back( y * width + x);
			}
		}

		// Add the constructed vector to the map
		_colorsMap.push_back(ledColors);

		totalCount += ledColors.size();
		totalCapacity += ledColors.capacity();

		ledCounter++;
	}
	Debug(_log, "Total index number is: %d (memory: %d). Reduced pixel set factor: %d, Accuracy level: %d, Image size: %d x %d, LED areas: %d",
		totalCount, totalCapacity, reducedPixelSetFactor, accuracyLevel, width, height, leds.size());

}

int ImageToLedsMap::width() const
{
	return _width;
}

int ImageToLedsMap::height() const
{
	return _height;
}

void ImageToLedsMap::setAccuracyLevel (int accuracyLevel)
{
	if (accuracyLevel > 4 )
	{
		Warning(_log, "Accuracy level %d is too high, it will be set to 4", accuracyLevel);
		accuracyLevel = 4;
	}
	//Set cluster number for dominant color advanced
	_clusterCount  = accuracyLevel + 1;

}

