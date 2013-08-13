
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
	ImageToLedsMap(const RgbImage& image, const std::vector<Led>& leds);

	/**
	 * Determines the mean-color for each led using the mapping the image given
	 * at construction.
	 *
	 * @return ledColors  The vector containing the output
	 */
	std::vector<RgbColor> getMeanLedColor();

	/**
	 * Determines the mean-color for each led using the mapping the image given
	 * at construction.
	 *
	 * @param[out] ledColors  The vector containing the output
	 */
	void getMeanLedColor(std::vector<RgbColor>& ledColors);

private:
	std::vector<std::vector<const RgbColor*> > mColorsMap;

	/**
	 * Finds the 'mean color' of the given list. This is the mean over each color-channel (red,
	 * green, blue)
	 *
	 * @param colors  The list with colors
	 *
	 * @return The mean of the given list of colors (or black when empty)
	 */
	RgbColor findMeanColor(const std::vector<const RgbColor*>& colors);
};

} // end namespace hyperion
