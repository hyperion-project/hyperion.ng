// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

ImageProcessorFactory& ImageProcessorFactory::getInstance()
{
	static ImageProcessorFactory instance;
	// Return the singleton instance
	return instance;
}

void ImageProcessorFactory::init(const LedString& ledString, const QJsonObject & blackborderConfig, int mappingType)
{
	_ledString = ledString;
	_blackborderConfig = blackborderConfig;
	_mappingType = mappingType;
}

ImageProcessor* ImageProcessorFactory::newImageProcessor() const
{
	ImageProcessor* ip = new ImageProcessor(_ledString, _blackborderConfig);
	ip->setLedMappingType(_mappingType);

	return ip;
}
