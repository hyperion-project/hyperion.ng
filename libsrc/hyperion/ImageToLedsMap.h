
#pragma once

// hyperion-utils includes
#include <utils/RgbImage.h>

// hyperion includes
#include <hyperion/LedString.h>

namespace hyperion
{

class ImageToLedsMap
{
public:

	/**
	 * Constructs an mapping from the colors in the image to each led based on the border
	 * definition given in the list of leds. The map holds pointers to the given image and its
	 * lifetime should never exceed that of the given image
	 *
	 * @param[in] image  The RGB image
	 * @param[in] leds   The list with led specifications
	 */
	ImageToLedsMap(const unsigned width, const unsigned height, const std::vector<Led> & leds);

	unsigned width() const;

	unsigned height() const;

	/**
	 * Determines the mean-color for each led using the mapping the image given
	 * at construction.
	 *
	 * @return ledColors  The vector containing the output
	 */
	std::vector<RgbColor> getMeanLedColor(const RgbImage & image) const;

	/**
	 * Determines the mean-color for each led using the mapping the image given
	 * at construction.
	 *
	 * @param[out] ledColors  The vector containing the output
	 */
	void getMeanLedColor(const RgbImage & image, std::vector<RgbColor> & ledColors) const;

private:
	const unsigned _width;
	const unsigned _height;
	std::vector<std::vector<unsigned> > mColorsMap;

	/**
	 * Finds the 'mean color' of the given list. This is the mean over each color-channel (red,
	 * green, blue)
	 *
	 * @param colors  The list with colors
	 *
	 * @return The mean of the given list of colors (or black when empty)
	 */
	RgbColor calcMeanColor(const RgbImage & image, const std::vector<unsigned> & colors) const;
};

} // end namespace hyperion
