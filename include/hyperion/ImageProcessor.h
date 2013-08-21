
#pragma once

// Utils includes
#include <utils/RgbImage.h>

#include <hyperion/LedString.h>
#include <hyperion/ImageProcessorFactory.h>

// Forward class declaration
namespace hyperion { class ImageToLedsMap; }

/**
 * The ImageProcessor translates an RGB-image to RGB-values for the leds. The processing is
 * performed in two steps. First the average color per led-region is computed. Second a
 * color-tranform is applied based on a gamma-correction.
 */
class ImageProcessor
{
public:
	~ImageProcessor();

	/**
	 * Specifies the width and height of 'incomming' images. This will resize the buffer-image to
	 * match the given size.
	 * NB All earlier obtained references will be invalid.
	 *
	 * @param[in] width   The new width of the buffer-image
	 * @param[in] height  The new height of the buffer-image
	 */
	void setSize(const unsigned width, const unsigned height);

	/**
	 * Processes the image to a list of led colors. This will update the size of the buffer-image
	 * if required and call the image-to-leds mapping to determine the mean color per led.
	 *
	 * @param[in] image  The image to translate to led values
	 *
	 * @return The color value per led
	 */
	std::vector<RgbColor> process(const RgbImage& image);

	/**
	 * Determines the led colors of the image in the buffer.
	 *
	 * @param[out] ledColors  The color value per led
	 */
	void process(const RgbImage& image, std::vector<RgbColor>& ledColors);

private:
	friend class ImageProcessorFactory;

	ImageProcessor(const LedString &ledString);

private:
	const LedString mLedString;

	hyperion::ImageToLedsMap* mImageToLeds;
};

