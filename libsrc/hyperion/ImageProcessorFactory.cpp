
// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

ImageProcessorFactory& ImageProcessorFactory::getInstance()
{
	static ImageProcessorFactory instance;
	// Return the singleton instance
	return instance;
}

void ImageProcessorFactory::init(const LedString& ledString)
{
	_ledString = ledString;
}

ImageProcessor* ImageProcessorFactory::newImageProcessor() const
{
	return new ImageProcessor(_ledString);
}
