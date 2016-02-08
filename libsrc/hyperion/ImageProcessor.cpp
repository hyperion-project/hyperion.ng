
// Hyperion includes
#include <hyperion/ImageProcessor.h>
#include <hyperion/ImageToLedsMap.h>

// Blacborder includes
#include <blackborder/BlackBorderProcessor.h>

using namespace hyperion;

//ImageProcessor::ImageProcessor(const LedString& ledString, bool enableBlackBorderDetector, uint8_t blackborderThreshold) :
ImageProcessor::ImageProcessor(const LedString& ledString, const Json::Value & blackborderConfig) :
	_ledString(ledString),
	_enableBlackBorderRemoval(blackborderConfig.get("enable", true).asBool()),
	_borderProcessor(new BlackBorderProcessor(blackborderConfig) ),
	_imageToLeds(nullptr)
{
	// empty
}

ImageProcessor::~ImageProcessor()
{
	delete _imageToLeds;
	delete _borderProcessor;
}

unsigned ImageProcessor::getLedCount() const
{
	return _ledString.leds().size();
}

void ImageProcessor::setSize(const unsigned width, const unsigned height)
{
	// Check if the existing buffer-image is already the correct dimensions
	if (_imageToLeds && _imageToLeds->width() == width && _imageToLeds->height() == height)
	{
		return;
	}

	// Clean up the old buffer and mapping
	delete _imageToLeds;

	// Construct a new buffer and mapping
	_imageToLeds = new ImageToLedsMap(width, height, 0, 0, _ledString.leds());
}

void ImageProcessor::enableBalckBorderDetector(bool enable)
{
	_enableBlackBorderRemoval = enable;
}

bool ImageProcessor::getScanParameters(size_t led, double &hscanBegin, double &hscanEnd, double &vscanBegin, double &vscanEnd) const
{
	if (led < _ledString.leds().size())
	{
		const Led & l = _ledString.leds()[led];
		hscanBegin = l.minX_frac;
		hscanEnd = l.maxX_frac;
		vscanBegin = l.minY_frac;
		vscanEnd = l.maxY_frac;
	}

	return false;
}

