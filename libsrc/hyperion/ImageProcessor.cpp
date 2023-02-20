
// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/ImageToLedsMap.h>

// Blackborder includes
#include <blackborder/BlackBorderProcessor.h>

#include <QSharedPointer>
#include <QRgb>

using namespace hyperion;

void ImageProcessor::registerProcessingUnit(
		int width,
		int height,
		int horizontalBorder,
		int verticalBorder)
{
	if (width > 0 && height > 0)
	{
		_imageToLedColors = QSharedPointer<ImageToLedsMap>(new ImageToLedsMap(
								_log,
								width,
								height,
								horizontalBorder,
								verticalBorder,
								_ledString.leds(),
								_reducedPixelSetFactorFactor,
								_accuraryLevel
								));
	}
	else
	{
		_imageToLedColors = QSharedPointer<ImageToLedsMap>(nullptr);
	}
}

// global transform method
int ImageProcessor::mappingTypeToInt(const QString& mappingType)
{
	if (mappingType == "unicolor_mean" )
	{
		return 1;
	}
	else if (mappingType == "multicolor_mean_squared" )
	{
		return 2;
	}
	else if (mappingType == "dominant_color" )
	{
		return 3;
	}
	else if (mappingType == "dominant_color_advanced" )
	{
		return 4;
	}
	return 0;
}
// global transform method
QString ImageProcessor::mappingTypeToStr(int mappingType)
{
	QString typeText;
	switch (mappingType) {
	case 1:
		typeText = "unicolor_mean";
		break;
	case 2:
		typeText = "multicolor_mean_squared";
		break;
	case 3:
		typeText = "dominant_color";
		break;
	case 4:
		typeText = "dominant_color_advanced";
		break;
	default:
		typeText = "multicolor_mean";
		break;
	}

	return typeText;
}

ImageProcessor::ImageProcessor(const LedString& ledString, Hyperion* hyperion)
	: QObject(hyperion)
	, _log(nullptr)
	, _ledString(ledString)
	, _borderProcessor(new BlackBorderProcessor(hyperion, this))
	, _imageToLedColors(nullptr)
	, _mappingType(0)
	, _userMappingType(0)
	, _hardMappingType(-1)
	, _accuraryLevel(0)
	, _reducedPixelSetFactorFactor(1)
	, _hyperion(hyperion)
{
	QString subComponent = hyperion->property("instance").toString();
	_log= Logger::getInstance("IMAGETOLED", subComponent);

	// init
	handleSettingsUpdate(settings::COLOR, _hyperion->getSetting(settings::COLOR));
	// listen for changes in color - ledmapping
	connect(_hyperion, &Hyperion::settingsChanged, this, &ImageProcessor::handleSettingsUpdate);
}

ImageProcessor::~ImageProcessor()
{
}

void ImageProcessor::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::COLOR)
	{
		const QJsonObject& obj = config.object();
		int newType = mappingTypeToInt(obj["imageToLedMappingType"].toString());
		if(_userMappingType != newType)
		{
			setLedMappingType(newType);
		}

		int reducedPixelSetFactorFactor = obj["reducedPixelSetFactorFactor"].toString().toInt();
		setReducedPixelSetFactorFactor(reducedPixelSetFactorFactor);

		int accuracyLevel = obj["accuracyLevel"].toInt();
		setAccuracyLevel(accuracyLevel);
	}
}

void ImageProcessor::setSize(int width, int height)
{
	// Check if the existing buffer-image is already the correct dimensions
	if (!_imageToLedColors.isNull() && _imageToLedColors->width() == width && _imageToLedColors->height() == height)
	{
		return;
	}

	// Construct a new buffer and mapping
	registerProcessingUnit(width, height, 0, 0);
}

void ImageProcessor::setLedString(const LedString& ledString)
{
	Debug(_log,"");
	if ( !_imageToLedColors.isNull() )
	{
		_ledString = ledString;

		// get current width/height
		int width = _imageToLedColors->width();
		int height = _imageToLedColors->height();

		// Construct a new buffer and mapping
		registerProcessingUnit(width, height, 0, 0);
	}
}

void ImageProcessor::setBlackbarDetectDisable(bool enable)
{
	_borderProcessor->setHardDisable(enable);
}

bool ImageProcessor::blackBorderDetectorEnabled() const
{
	return _borderProcessor->enabled();
}

void ImageProcessor::setReducedPixelSetFactorFactor(int count)
{
	int currentReducedPixelSetFactor= _reducedPixelSetFactorFactor;

	_reducedPixelSetFactorFactor = count;
	Debug(_log, "Set reduced pixel set factor to %d", _reducedPixelSetFactorFactor);

	if (currentReducedPixelSetFactor != _reducedPixelSetFactorFactor && !_imageToLedColors.isNull())
	{
		int width = _imageToLedColors->width();
		int height = _imageToLedColors->height();

		// Construct a new buffer and mapping
		registerProcessingUnit(width, height, 0, 0);
	}
}

void ImageProcessor::setAccuracyLevel(int level)
{
	_accuraryLevel = level;
	Debug(_log, "Set processing accuracy level to %d", _accuraryLevel);

	if (!_imageToLedColors.isNull())
	{
		_imageToLedColors->setAccuracyLevel(_accuraryLevel);
	}
}

void ImageProcessor::setLedMappingType(int mapType)
{
	int currentMappingType = _mappingType;

	// if the _hardMappingType is >-1 we aren't allowed to overwrite it
	_userMappingType = mapType;

	Debug(_log, "Set user LED mapping to %s", QSTRING_CSTR(mappingTypeToStr(mapType)));

	if(_hardMappingType == -1)
	{
		_mappingType = mapType;
	}

	if (currentMappingType != _mappingType && !_imageToLedColors.isNull())
	{
		int width = _imageToLedColors->width();
		int height = _imageToLedColors->height();

		registerProcessingUnit(width, height, 0, 0);
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
