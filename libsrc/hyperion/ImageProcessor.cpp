#include <hyperion/ImageProcessor.h>


#include "ImageToLedsMap.h"
#include "ColorTransform.h"

using namespace hyperion;

ImageProcessor::ImageProcessor(const LedString& ledString) :
	mLedString(ledString),
	mBuffer(nullptr),
	mImageToLeds(nullptr)
{
	// empty
}

ImageProcessor::~ImageProcessor()
{
	delete mImageToLeds;
	delete mBuffer;
}

std::vector<RgbColor> ImageProcessor::process(const RgbImage& image)
{
	// Ensure that the buffer-image is the proper size
	setSize(image.width(), image.height());

	// Copy the data of the given image into the mapped-image
	mBuffer->copy(image);

	// Create a result vector and call the 'in place' functionl
	std::vector<RgbColor> colors(mLedString.leds().size(), RgbColor::BLACK);
	inplace_process(colors);

	// return the computed colors
	return colors;
}

void ImageProcessor::setSize(const unsigned width, const unsigned height)
{
	// Check if the existing buffer-image is already the correct dimensions
	if (mBuffer && mBuffer->width() == width && mBuffer->height() == height)
	{
		return;
	}

	// Clean up the old buffer and mapping
	delete mImageToLeds;
	delete mBuffer;

	// Construct a new buffer and mapping
	mBuffer = new RgbImage(width, height);
	mImageToLeds = new ImageToLedsMap(*mBuffer, mLedString.leds());
}

RgbImage& ImageProcessor::image()
{
	return *mBuffer;
}

void ImageProcessor::inplace_process(std::vector<RgbColor>& ledColors)
{
	// Determine the mean-colors of each led (using the existing mapping)
	mImageToLeds->getMeanLedColor(ledColors);
}
