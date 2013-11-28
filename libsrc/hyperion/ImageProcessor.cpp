
// Hyperion includes
#include <hyperion/ImageProcessor.h>
#include <hyperion/ImageToLedsMap.h>
#include <hyperion/BlackBorderProcessor.h>

#include <utils/ColorTransform.h>

using namespace hyperion;

ImageProcessor::ImageProcessor(const LedString& ledString, bool enableBlackBorderDetector) :
	mLedString(ledString),
	_enableBlackBorderRemoval(enableBlackBorderDetector),
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

unsigned ImageProcessor::getLedCount() const
{
	return mLedString.leds().size();
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

bool ImageProcessor::getScanParameters(size_t led, double &hscanBegin, double &hscanEnd, double &vscanBegin, double &vscanEnd) const
{
	if (led < mLedString.leds().size())
	{
		const Led & l = mLedString.leds()[led];
		hscanBegin = l.minX_frac;
		hscanEnd = l.maxX_frac;
		vscanBegin = l.minY_frac;
		vscanEnd = l.maxY_frac;
	}

	return false;
}

