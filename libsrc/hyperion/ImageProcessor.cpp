
// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ImageToLedsMap.h>

// Blacborder includes
#include <blackborder/BlackBorderProcessor.h>

using namespace hyperion;

// global transform method
int ImageProcessor::mappingTypeToInt(QString mappingType)
{
	if (mappingType == "unicolor_mean" )
		return 1;

	return 0;
}
// global transform method
QString ImageProcessor::mappingTypeToStr(int mappingType)
{
	if (mappingType == 1 )
		return "unicolor_mean";

	return "multicolor_mean";
}

ImageProcessor::ImageProcessor(const LedString& ledString, Hyperion* hyperion)
	: QObject(hyperion)
	, _log(Logger::getInstance("BLACKBORDER"))
	, _ledString(ledString)
	, _borderProcessor(new BlackBorderProcessor(hyperion, this))
	, _imageToLeds(nullptr)
	, _mappingType(0)
	, _userMappingType(0)
	, _hardMappingType(0)
	, _hyperion(hyperion)
{
	// init
	handleSettingsUpdate(settings::COLOR, _hyperion->getSetting(settings::COLOR));
	// listen for changes in color - ledmapping
	connect(_hyperion, &Hyperion::settingsChanged, this, &ImageProcessor::handleSettingsUpdate);
}

ImageProcessor::~ImageProcessor()
{
	delete _imageToLeds;
}

void ImageProcessor::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::COLOR)
	{
		const QJsonObject& obj = config.object();
		int newType = mappingTypeToInt(obj["imageToLedMappingType"].toString());
		if(_userMappingType != newType)
		{
			setLedMappingType(newType);
		}
	}
}

void ImageProcessor::setSize(const unsigned width, const unsigned height)
{
	// Check if the existing buffer-image is already the correct dimensions
	if (_imageToLeds && _imageToLeds->width() == width && _imageToLeds->height() == height)
	{
		return;
	}

	// Clean up the old buffer and mapping
	_imageToLeds = 0;

	// Construct a new buffer and mapping
	_imageToLeds = (width>0 && height>0) ? (new ImageToLedsMap(width, height, 0, 0, _ledString.leds())) : nullptr;
}

void ImageProcessor::setLedString(const LedString& ledString)
{
	_ledString = ledString;

	// get current width/height
	const unsigned width = _imageToLeds->width();
	const unsigned height = _imageToLeds->height();

	// Clean up the old buffer and mapping
	_imageToLeds = 0;

	// Construct a new buffer and mapping
	_imageToLeds = new ImageToLedsMap(width, height, 0, 0, _ledString.leds());
}

void ImageProcessor::setBlackbarDetectDisable(bool enable)
{
	_borderProcessor->setHardDisable(enable);
}

bool ImageProcessor::blackBorderDetectorEnabled()
{
	return _borderProcessor->enabled();
}

void ImageProcessor::setLedMappingType(int mapType)
{
	// if the _hardMappingType is >-1 we aren't allowed to overwrite it
	_userMappingType = mapType;
	Debug(_log, "set user led mapping to %s", QSTRING_CSTR(mappingTypeToStr(mapType)));
	if(_hardMappingType == -1)
	{
		_mappingType = mapType;
	}
}

void ImageProcessor::setHardLedMappingType(int mapType)
{
	// force the maptype, if set to -1 we use the last requested _userMappingType
	_hardMappingType = mapType;
	if(mapType == -1)
		_mappingType = _userMappingType;
	else
		_mappingType = mapType;
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
