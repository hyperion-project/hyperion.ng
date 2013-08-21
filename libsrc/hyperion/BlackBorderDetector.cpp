#include "BlackBorderDetector.h"

BlackBorderDetector::BlackBorderDetector()
{
	// empty
}

BlackBorder BlackBorderDetector::process(const RgbImage& image)
{
	int firstNonBlackPixelTop  = -1;
	int firstNonBlackPixelLeft = -1;

	// Find the non-black pixel at the top-half border
	for (unsigned x=0; x<image.width()/2; ++x)
	{
		const RgbColor& color = image(x, 0);
		if (!isBlack(color))
		{
			firstNonBlackPixelTop = x;
			break;
		}
	}
	// Find the non-black pixel at the left-half border
	for (unsigned y=0; y<image.height()/2; ++y)
	{
		const RgbColor& color = image(0, y);
		if (!isBlack(color))
		{
			firstNonBlackPixelLeft = y;
			break;
		}
	}

	// Construct 'unknown' result
	BlackBorder detectedBorder;
	detectedBorder.type = BlackBorder::unknown;

	if (firstNonBlackPixelTop == 0 /*&& firstNonBlackPixelLeft == 0*/)
	{
		// No black border
		// C-?-?-? ...
		// ? +----
		// ? |
		// ? |
		// :

		detectedBorder.type = BlackBorder::none;
		detectedBorder.size = -1;
	}
	else if (firstNonBlackPixelTop < 0)
	{
		if (firstNonBlackPixelLeft < 0)
		{
			// We don't know
			// B-B-B-B ...
			// B +---- ...
			// B |
			// B |
			// :

			detectedBorder.type = BlackBorder::unknown;
			detectedBorder.size = -1;
		}
		else //(firstNonBlackPixelLeft > 0)
		{
			// Border at top of screen
			// B-B-B-B ...
			// B +---- ...
			// C |
			// ? |
			// :

			detectedBorder.type = BlackBorder::horizontal;
			detectedBorder.size = firstNonBlackPixelLeft;
		}
	}
	else // (firstNonBlackPixelTop > 0)
	{
		if (firstNonBlackPixelLeft < 0)
		{
			// Border at left of screen
			// B-B-C-? ...
			// B +---- ...
			// B |
			// B |
			// :

			detectedBorder.type = BlackBorder::vertical;
			detectedBorder.size = firstNonBlackPixelTop;
		}
		else //(firstNonBlackPixelLeft > 0)
		{
			// No black border
			// B-B-C-? ...
			// B +----
			// C |
			// ? |
			// :

			detectedBorder.type = BlackBorder::none;
			detectedBorder.size = -1;
		}
	}

	return detectedBorder;
}
