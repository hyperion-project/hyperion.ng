// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

ImageProcessorFactory& ImageProcessorFactory::getInstance()
{
	static ImageProcessorFactory instance;
	// Return the singleton instance
	return instance;
}

void ImageProcessorFactory::init(const LedString& ledString, const Json::Value & blackborderConfig)
{
	_ledString = ledString;
	_blackborderConfig = blackborderConfig;
}

ImageProcessor* ImageProcessorFactory::newImageProcessor() const
{
	return new ImageProcessor(_ledString, _blackborderConfig);
}
