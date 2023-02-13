#pragma once

#include <QString>
#include <QSharedPointer>

// Utils includes
#include <utils/Image.h>

// Hyperion includes
#include <hyperion/LedString.h>
#include <hyperion/ImageToLedsMap.h>
#include <utils/Logger.h>

// settings
#include <utils/settings.h>

// Black border includes
#include <blackborder/BlackBorderProcessor.h>

class Hyperion;

///
/// The ImageProcessor translates an RGB-image to RGB-values for the LEDs. The processing is
/// performed in two steps. First the average color per led-region is computed. Second a
/// color-transform is applied based on a gamma-correction.
///
class ImageProcessor : public QObject
{
	Q_OBJECT

public:
	///
	/// Constructs an image-processor for translating an image to led-color values based on the
	/// given led-string specification
	///	@param[in] ledString    LedString data
	/// @param[in] hyperion     Hyperion instance pointer
	///
	ImageProcessor(const LedString& ledString, Hyperion* hyperion);

	~ImageProcessor() override;

	///
	/// Specifies the width and height of 'incoming' images. This will resize the buffer-image to
	/// match the given size.
	/// NB All earlier obtained references will be invalid.
	///
	/// @param[in] width   The new width of the buffer-image
	/// @param[in] height  The new height of the buffer-image
	///
	void setSize(int width, int height);

	///
	/// @brief Update the led string (eg on settings change)
	///
	void setLedString(const LedString& ledString);

	/// Returns state of black border detector
	bool blackBorderDetectorEnabled() const;

	///
	///  Factor to reduce the number of pixels evaluated during processing
	///
	/// @param[in] count  Use every "count" pixel
	void setReducedPixelSetFactorFactor(int count);

	///
	/// Set the accuracy used during processing
	/// (only for selected types)
	///
	/// @param[in] level  The accuracy level (0-4)
	void setAccuracyLevel(int level);

	/// Returns the current _userMappingType, this may not be the current applied type!
	int getUserLedMappingType() const { return _userMappingType; }

	/// Returns the current _mappingType
	int ledMappingType() const { return _mappingType; }

	static int mappingTypeToInt(const QString& mappingType);
	static QString mappingTypeToStr(int mappingType);

	///
	/// @brief Set the Hyperion::update() request LED mapping type. This type is used in favour of type set with setLedMappingType.
	/// 	   If you don't want to force a mapType set this to -1 (user choice will be set)
	/// @param  mapType   The new mapping type
	///
	void setHardLedMappingType(int mapType);

public slots:
	/// Enable or disable the black border detector based on component
	void setBlackbarDetectDisable(bool enable);

	///
	/// @brief Set the user requested led mapping.
	/// 	   The type set with setHardLedMappingType() will be used in favour to respect comp specific settings
	/// @param  mapType   The new mapping type
	///
	void setLedMappingType(int mapType);

public:
	///
	/// Specifies the width and height of 'incoming' images. This will resize the buffer-image to
	/// match the given size.
	/// NB All earlier obtained references will be invalid.
	///
	/// @param[in] image   The dimensions taken from image
	///
	template <typename Pixel_T>
	void setSize(const Image<Pixel_T> &image)
	{
		setSize(image.width(), image.height());
	}

	///
	/// Processes the image to a list of LED colors. This will update the size of the buffer-image
	/// if required and call the image-to-LEDs mapping to determine the color per LED.
	///
	/// @param[in] image  The image to translate to LED values
	///
	/// @return The color value per LED
	///
	template <typename Pixel_T>
	std::vector<ColorRgb> process(const Image<Pixel_T>& image)
	{
		std::vector<ColorRgb> colors;

		if (image.width()>0 && image.height()>0)
		{
			// Ensure that the buffer-image is the proper size
			setSize(image);

			assert(!_imageToLedColors.isNull());

			// Check black border detection
			verifyBorder(image);

			// Create a result vector and call the 'in place' function
			switch (_mappingType)
			{
			case 1:
				colors = _imageToLedColors->getUniLedColor(image);
				break;
			case 2:
				colors = _imageToLedColors->getMeanLedColorSqrt(image);
				break;
			case 3:
				colors = _imageToLedColors->getDominantLedColor(image);
				break;
			case 4:
				colors = _imageToLedColors->getDominantLedColorAdv(image);
				break;
			default:
				colors = _imageToLedColors->getMeanLedColor(image);
			}
		}
		else
		{
			Warning(_log, "ImageProcessor::process called with image size 0");
		}

		// return the computed colors
		return colors;
	}

	///
	/// Determines the led colors of the image in the buffer.
	///
	/// @param[in] image  The image to translate to LED values
	/// @param[out] ledColors  The color value per LED
	///
	template <typename Pixel_T>
	void process(const Image<Pixel_T>& image, std::vector<ColorRgb>& ledColors)
	{
		if ( image.width()>0 && image.height()>0)
		{
			// Ensure that the buffer-image is the proper size
			setSize(image);

			// Check black border detection
			verifyBorder(image);

			// Determine the mean or uni colors of each led (using the existing mapping)
			switch (_mappingType)
			{
			case 1:
				_imageToLedColors->getUniLedColor(image, ledColors);
				break;
			case 2:
				_imageToLedColors->getMeanLedColorSqrt(image, ledColors);
				break;
			case 3:
				_imageToLedColors->getDominantLedColor(image, ledColors);
				break;
			case 4:
				_imageToLedColors->getDominantLedColorAdv(image, ledColors);
				break;
			default:
				_imageToLedColors->getMeanLedColor(image, ledColors);
			}
		}
		else
		{
			Warning(_log, "Called with image size 0");
		}
	}

	///
	/// Get the hscan and vscan parameters for a single LED
	///
	/// @param[in] led Index of the LED
	/// @param[out] hscanBegin begin of the hscan
	/// @param[out] hscanEnd end of the hscan
	/// @param[out] vscanBegin begin of the hscan
	/// @param[out] vscanEnd end of the hscan
	/// @return true if the parameters could be retrieved
	bool getScanParameters(size_t led, double & hscanBegin, double & hscanEnd, double & vscanBegin, double & vscanEnd) const;

private:

	void registerProcessingUnit(
		int width,
		int height,
		int horizontalBorder,
		int verticalBorder);

	///
	/// Performs black-border detection (if enabled) on the given image
	///
	/// @param[in] image  The image to perform black-border detection on
	///
	template <typename Pixel_T>
	void verifyBorder(const Image<Pixel_T> & image)
	{
		if (!_borderProcessor->enabled() && ( _imageToLedColors->horizontalBorder()!=0 || _imageToLedColors->verticalBorder()!=0 ))
		{
			Debug(_log, "Reset border");
			_borderProcessor->process(image);
			registerProcessingUnit(image.width(), image.height(), 0, 0);
		}

		if(_borderProcessor->enabled() && _borderProcessor->process(image))
		{
			const hyperion::BlackBorder border = _borderProcessor->getCurrentBorder();

			if (border.unknown)
			{
				registerProcessingUnit(image.width(), image.height(), 0, 0);
			}
			else
			{
				registerProcessingUnit(image.width(), image.height(), border.horizontalSize, border.verticalSize);
			}
		}
	}

private slots:
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

private:

	Logger * _log;
	/// The Led-string specification
	LedString _ledString;

	/// The processor for black border detection
	hyperion::BlackBorderProcessor * _borderProcessor;

	/// The mapping of image-pixels to LEDs
	QSharedPointer<hyperion::ImageToLedsMap> _imageToLedColors;

	/// Type of image to LED mapping
	int _mappingType;
	/// Type of last requested user type
	int _userMappingType;
	/// Type of last requested hard type
	int _hardMappingType;

	int _accuraryLevel;
	int _reducedPixelSetFactorFactor;

	/// Hyperion instance pointer
	Hyperion* _hyperion;
};
