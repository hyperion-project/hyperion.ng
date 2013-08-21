#include "BlackBorderDetector.h"

BlackBorderDetector::BlackBorderDetector()
{
}

BlackBorder BlackBorderDetector::process(const RgbImage& image)
{
	int firstNonBlackPixelTop  = -1;
	int firstNonBlackPixelLeft = -1;

	for (unsigned x=0; x<image.width(); ++x)
	{
		const RgbColor& color = image(x, 0);
		if (!isBlack(color))
		{
			firstNonBlackPixelTop = x;
			break;
		}
	}
	for (unsigned y=0; y<image.height(); ++y)
	{
		const RgbColor& color = image(0, y);
		if (!isBlack(color))
		{
			firstNonBlackPixelLeft = y;
			break;
		}
	}

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
		if (firstNonBlackPixelLeft < 0 || firstNonBlackPixelLeft > (int)(image.height()/2) )
		{
			// We don't know
			// B-B-B-B ... B-B-B-B
			// B +---- ... ----- ?
			// B |
			// B |
			// :
			// B |
			// B |
			// B |
			// B ?

			detectedBorder.type = BlackBorder::unknown;
			detectedBorder.size = -1;
		}
		else //(firstNonBlackPixelLeft > 0 && firstNonBlackPixelLeft < image.height()/2)
		{
			// Border at top of screen
			// B-B-B-B ... B-B-B-B
			// B +---- ... ----- ?
			// C |
			// ? |
			// :

			detectedBorder.type = BlackBorder::horizontal;
			detectedBorder.size = firstNonBlackPixelLeft;
		}
	}
	else // (firstNonBlackPixelTop > 0)
	{
		if (firstNonBlackPixelTop < int(image.width()/2) && firstNonBlackPixelLeft < 0)
		{
			// Border at left of screen
			// B-B-C-? ...
			// B +---- ... ----- ?
			// B |
			// B |
			// :
			// B |
			// B |
			// B |
			// B ?

			detectedBorder.type = BlackBorder::vertical;
			detectedBorder.size = firstNonBlackPixelTop;
		}
		else //(firstNonBlackPixelTop > int(mage.width()/2) || firstNonBlackPixelLeft > 0)
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
