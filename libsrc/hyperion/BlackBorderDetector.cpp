
// Local-Hyperion includes
#include "BlackBorderDetector.h"

using namespace hyperion;

BlackBorderDetector::BlackBorderDetector()
{
	// empty
}

BlackBorder BlackBorderDetector::process(const RgbImage& image)
{
	// only test the topleft third of the image
	int width = image.width() /3;
	int height = image.height() / 3;
	int maxSize = std::max(width, height);

	int firstNonBlackXPixelIndex = -1;
	int firstNonBlackYPixelIndex = -1;

	// find some pixel of the image
	for (int i = 0; i < maxSize; ++i)
	{
		int x = std::min(i, width);
		int y = std::min(i, height);

		const RgbColor& color = image(x, y);
		if (!isBlack(color))
		{
			firstNonBlackXPixelIndex = x;
			firstNonBlackYPixelIndex = y;
			break;
		}
	}

	// expand image to the left
	for(; firstNonBlackXPixelIndex > 0; --firstNonBlackXPixelIndex)
	{
		const RgbColor& color = image(firstNonBlackXPixelIndex-1, firstNonBlackYPixelIndex);
		if (isBlack(color))
		{
			break;
		}
	}

	// expand image to the top
	for(; firstNonBlackYPixelIndex > 0; --firstNonBlackYPixelIndex)
	{
		const RgbColor& color = image(firstNonBlackXPixelIndex, firstNonBlackYPixelIndex-1);
		if (isBlack(color))
		{
			break;
		}
	}

	// Construct result
	BlackBorder detectedBorder;
	detectedBorder.unknown = firstNonBlackXPixelIndex == -1 || firstNonBlackYPixelIndex == -1;
	detectedBorder.horizontalSize = firstNonBlackYPixelIndex;
	detectedBorder.verticalSize = firstNonBlackXPixelIndex;
	return detectedBorder;
}
