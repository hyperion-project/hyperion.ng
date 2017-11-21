#pragma once

#include <QString>

// Utils includes
#include <utils/Image.h>

// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/LedString.h>
#include <hyperion/ImageToLedsMap.h>
#include <utils/Logger.h>

// Black border includes
#include <blackborder/BlackBorderProcessor.h>

///
/// The ImageProcessor translates an RGB-image to RGB-values for the leds. The processing is
/// performed in two steps. First the average color per led-region is computed. Second a
/// color-tranform is applied based on a gamma-correction.
///
class ImageProcessor : public QObject
{
	Q_OBJECT

public:

	~ImageProcessor();

	///
	/// Returns the number of attached leds
	///
	unsigned getLedCount() const;

	///
	/// Specifies the width and height of 'incomming' images. This will resize the buffer-image to
	/// match the given size.
	/// NB All earlier obtained references will be invalid.
	///
	/// @param[in] width   The new width of the buffer-image
	/// @param[in] height  The new height of the buffer-image
	///
	void setSize(const unsigned width, const unsigned height);

	/// Returns starte of black border detector
	bool blackBorderDetectorEnabled();

	/// Returns starte of black border detector
	int ledMappingType();

	static int mappingTypeToInt(QString mappingType);
	static QString mappingTypeToStr(int mappingType);

public slots:
	/// Enable or disable the black border detector
	void enableBlackBorderDetector(bool enable);

	/// Enable or disable the black border detector
	void setLedMappingType(int mapType);

public:
	///
	/// Specifies the width and height of 'incomming' images. This will resize the buffer-image to
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
	/// Processes the image to a list of led colors. This will update the size of the buffer-image
	/// if required and call the image-to-leds mapping to determine the mean color per led.
	///
	/// @param[in] image  The image to translate to led values
	///
	/// @return The color value per led
	///
	template <typename Pixel_T>
	std::vector<ColorRgb> process(const Image<Pixel_T>& image)
	{
		std::vector<ColorRgb> colors;
		if (image.width()>0 && image.height()>0)
		{
			// Ensure that the buffer-image is the proper size
			setSize(image);

			// Check black border detection
			verifyBorder(image);

			// Create a result vector and call the 'in place' functionl
			switch (_mappingType)
			{
				case 1: colors = _imageToLeds->getUniLedColor(image); break;
				default: colors = _imageToLeds->getMeanLedColor(image);
			}
		}
		else
		{
			Warning(_log, "ImageProcessor::process called without image size 0");
		}

		// return the computed colors
		return colors;
	}

	///
	/// Determines the led colors of the image in the buffer.
	///
	/// @param[in] image  The image to translate to led values
	/// @param[out] ledColors  The color value per led
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

			// Determine the mean-colors of each led (using the existing mapping)
			switch (_mappingType)
			{
				case 1: _imageToLeds->getUniLedColor(image, ledColors); break;
				default: _imageToLeds->getMeanLedColor(image, ledColors);
			}
		}
		else
		{
			Warning(_log, "ImageProcessor::process called without image size 0");
		}
	}

	///
	/// Get the hscan and vscan parameters for a single led
	///
	/// @param[in] led Index of the led
	/// @param[out] hscanBegin begin of the hscan
	/// @param[out] hscanEnd end of the hscan
	/// @param[out] vscanBegin begin of the hscan
	/// @param[out] vscanEnd end of the hscan
	/// @return true if the parameters could be retrieved
	bool getScanParameters(size_t led, double & hscanBegin, double & hscanEnd, double & vscanBegin, double & vscanEnd) const;

private:
	/// Friend declaration of the factory for creating ImageProcessor's
	friend class ImageProcessorFactory;

	///
	/// Constructs an image-processor for translating an image to led-color values based on the
	/// given led-string specification
	///
	/// @param[in] ledString  The led-string specification
	/// @param[in] blackborderThreshold The threshold which the blackborder detector should use
	///
	ImageProcessor(const LedString &ledString, const QJsonObject &blackborderConfig);

	///
	/// Performs black-border detection (if enabled) on the given image
	///
	/// @param[in] image  The image to perform black-border detection on
	///
	template <typename Pixel_T>
	void verifyBorder(const Image<Pixel_T> & image)
	{
		if (!_borderProcessor->enabled() && ( _imageToLeds->horizontalBorder()!=0 || _imageToLeds->verticalBorder()!=0 ))
		{
			Debug(Logger::getInstance("BLACKBORDER"), "disabled, reset border");
			_borderProcessor->process(image);
			delete _imageToLeds;
			_imageToLeds = new hyperion::ImageToLedsMap(image.width(), image.height(), 0, 0, _ledString.leds());
		}

		if(_borderProcessor->enabled() && _borderProcessor->process(image))
		{
			//Debug(Logger::getInstance("BLACKBORDER"), "BORDER SWITCH REQUIRED!!");

			const hyperion::BlackBorder border = _borderProcessor->getCurrentBorder();

			// Clean up the old mapping
			delete _imageToLeds;

			if (border.unknown)
			{
				// Construct a new buffer and mapping
				_imageToLeds = new hyperion::ImageToLedsMap(image.width(), image.height(), 0, 0, _ledString.leds());
			}
			else
			{
				// Construct a new buffer and mapping
				_imageToLeds = new hyperion::ImageToLedsMap(image.width(), image.height(), border.horizontalSize, border.verticalSize, _ledString.leds());
			}

			//Debug(Logger::getInstance("BLACKBORDER"),  "CURRENT BORDER TYPE: unknown=%d hor.size=%d vert.size=%d",
			//	border.unknown, border.horizontalSize, border.verticalSize );
		}
	}

private:
	Logger * _log;
	/// The Led-string specification
	const LedString _ledString;

	/// The processor for black border detection
	hyperion::BlackBorderProcessor * _borderProcessor;

	/// The mapping of image-pixels to leds
	hyperion::ImageToLedsMap* _imageToLeds;

	/// Type of image 2 led mapping
	int _mappingType;
};
