
// STL includes
#include <cmath>

// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

ImageProcessorFactory& ImageProcessorFactory::getInstance()
{
	static ImageProcessorFactory instance;
	// Return the singleton instance
	return instance;
}

void ImageProcessorFactory::init(const LedString& ledString, bool enableBlackBorderDetector, double blackborderThreshold)
{
	_ledString = ledString;
	_enableBlackBorderDetector = enableBlackBorderDetector;

	int threshold = int(std::ceil(blackborderThreshold * 255));
	if (threshold < 0)
		threshold = 0;
	else if (threshold > 255)
		threshold = 255;
	_blackborderThreshold = uint8_t(threshold);

	if (_enableBlackBorderDetector)
	{
		std::cout << "Black border threshold set to " << blackborderThreshold << " (" << int(_blackborderThreshold) << ")" << std::endl;
	}
}

ImageProcessor* ImageProcessorFactory::newImageProcessor() const
{
	return new ImageProcessor(_ledString, _enableBlackBorderDetector, _blackborderThreshold);
}
