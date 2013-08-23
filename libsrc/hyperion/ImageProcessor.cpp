
// Hyperion includes
#include <hyperion/ImageProcessor.h>

// Local-Hyperion includes
#include "BlackBorderProcessor.h"
#include "ColorTransform.h"
#include "ImageToLedsMap.h"

using namespace hyperion;

ImageProcessor::ImageProcessor(const LedString& ledString) :
	mLedString(ledString),
	_enableBlackBorderRemoval(true),
	_borderProcessor(new BlackBorderProcessor(600, 50, 1)),
	mImageToLeds(nullptr)
{
	// empty
}

ImageProcessor::~ImageProcessor()
{
	delete mImageToLeds;
	delete _borderProcessor;
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

	// Check black border detection
	verifyBorder(image);

	// Create a result vector and call the 'in place' functionl
	std::vector<RgbColor> colors = mImageToLeds->getMeanLedColor(image);

	// return the computed colors
	return colors;
}

void ImageProcessor::process(const RgbImage& image, std::vector<RgbColor>& ledColors)
{
	// Check black border detection
	verifyBorder(image);

	// Determine the mean-colors of each led (using the existing mapping)
	mImageToLeds->getMeanLedColor(image, ledColors);
}

void ImageProcessor::verifyBorder(const RgbImage& image)
{
	if(_enableBlackBorderRemoval && _borderProcessor->process(image))
	{
		std::cout << "BORDER SWITCH REQUIRED!!" << std::endl;

		const BlackBorder border = _borderProcessor->getCurrentBorder();

		// Clean up the old mapping
		delete mImageToLeds;

		switch (border.type)
		{
		case BlackBorder::none:
		case BlackBorder::unknown:
			// Construct a new buffer and mapping
			mImageToLeds = new ImageToLedsMap(image.width(), image.height(), 0, 0, mLedString.leds());
			break;
		case BlackBorder::horizontal:
			// Construct a new buffer and mapping
			mImageToLeds = new ImageToLedsMap(image.width(), image.height(), border.size, 0, mLedString.leds());
			break;
		case BlackBorder::vertical:
			// Construct a new buffer and mapping
			mImageToLeds = new ImageToLedsMap(image.width(), image.height(), 0, border.size, mLedString.leds());
			break;
		}

		std::cout << "CURRENT BORDER TYPE: " << _borderProcessor->getCurrentBorder().type << " (size=" << _borderProcessor->getCurrentBorder().size << ")" << std::endl;
	}

}
