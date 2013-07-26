
// STL includes
#include <algorithm>

// hyperion includes
#include <hyperion/ImageToLedsMap.h>

ImageToLedsMap::ImageToLedsMap()
{
	// empty
}

void ImageToLedsMap::createMapping(const RgbImage& image, const std::vector<Led>& leds)
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

std::vector<RgbColor> ImageToLedsMap::getMedianLedColor()
{
	std::vector<RgbColor> ledColors;
	for (std::vector<const RgbColor*>& colors : mColorsMap)
	{
		const RgbColor color = findMedianColor(colors);
		ledColors.push_back(color);
	}

	return ledColors;
}

RgbColor ImageToLedsMap::findMedianColor(std::vector<const RgbColor*>& colors)
{
	std::sort(colors.begin(), colors.end(), [](const RgbColor* lhs, const RgbColor* rhs){ return lhs->red   < rhs->red;   });
	const uint8_t red   = colors.at(colors.size()/2)->red;
	std::sort(colors.begin(), colors.end(), [](const RgbColor* lhs, const RgbColor* rhs){ return lhs->green < rhs->green; });
	const uint8_t green = colors.at(colors.size()/2)->green;
	std::sort(colors.begin(), colors.end(), [](const RgbColor* lhs, const RgbColor* rhs){ return lhs->blue  < rhs->blue;  });
	const uint8_t blue  = colors.at(colors.size()/2)->blue;

	return {red, green, blue};
}
