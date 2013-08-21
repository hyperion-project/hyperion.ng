#include <hyperion/ImageProcessor.h>


#include "ImageToLedsMap.h"
#include "ColorTransform.h"

using namespace hyperion;

ImageProcessor::ImageProcessor(const LedString& ledString) :
	mLedString(ledString),
	mImageToLeds(nullptr)
{
	// empty
}

ImageProcessor::~ImageProcessor()
{
	delete mImageToLeds;
}

void ImageProcessor::setSize(const unsigned width, const unsigned height)
{
	// Check if the existing buffer-image is already the correct dimensions
	if (mImageToLeds && mImageToLeds->width() == width && mImageToLeds->height() == height)
	{
		return;
	}

	// Clean up the old buffer and mapping
	delete mImageToLeds;

	// Construct a new buffer and mapping
	mImageToLeds = new ImageToLedsMap(width, height, 0, 0, mLedString.leds());
}

std::vector<RgbColor> ImageProcessor::process(const RgbImage& image)
{
	// Ensure that the buffer-image is the proper size
	setSize(image.width(), image.height());

	// Create a result vector and call the 'in place' functionl
	std::vector<RgbColor> colors = mImageToLeds->getMeanLedColor(image);

	// return the computed colors
	return colors;
}

void ImageProcessor::process(const RgbImage& image, std::vector<RgbColor>& ledColors)
{
	// Determine the mean-colors of each led (using the existing mapping)
	mImageToLeds->getMeanLedColor(image, ledColors);
}
