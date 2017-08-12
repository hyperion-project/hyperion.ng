
// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ImageToLedsMap.h>

// Blacborder includes
#include <blackborder/BlackBorderProcessor.h>

using namespace hyperion;

ImageProcessor::ImageProcessor(const LedString& ledString, const QJsonObject & blackborderConfig)
	: QObject()
	, _log(Logger::getInstance("BLACKBORDER"))
	, _ledString(ledString)
	, _borderProcessor(new BlackBorderProcessor(blackborderConfig) )
	, _imageToLeds(nullptr)
	, _mappingType(0)
{
// this is when we want to change the mapping for all input sources
// 	connect(Hyperion::getInstance(), SIGNAL(imageToLedsMappingChanged(int)), this, SLOT(setLedMappingType(int))); 
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
	_imageToLeds = (width>0 && height>0) ? (new ImageToLedsMap(width, height, 0, 0, _ledString.leds())) : nullptr;
}

void ImageProcessor::enableBlackBorderDetector(bool enable)
{
	_borderProcessor->setEnabled(enable);
}

bool ImageProcessor::blackBorderDetectorEnabled()
{
	return _borderProcessor->enabled();
}

void ImageProcessor::setLedMappingType(int mapType)
{
	Debug(_log, "set led mapping to type %d", mapType);
	_mappingType = mapType;
}

int ImageProcessor::ledMappingType()
{
	return _mappingType;
}

int ImageProcessor::mappingTypeToInt(QString mappingType)
{
	if (mappingType == "unicolor_mean" )
		return 1;

	return 0;
}

QString ImageProcessor::mappingTypeToStr(int mappingType)
{
	if (mappingType == 1 )
		return "unicolor_mean";

	return "multicolor_mean";
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

