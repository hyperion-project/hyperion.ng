
#pragma once

// STL includes
#include <sstream>

// hyperion-utils includes
#include <utils/RgbImage.h>

// hyperion includes
#include <hyperion/LedString.h>

namespace hyperion
{

	///
	/// The ImageToLedsMap holds a mapping of indices into an image to leds. It can be used to
	/// calculate the average (or mean) color per led for a specific region.
	///
	class ImageToLedsMap
	{
	public:

		///
		/// Constructs an mapping from the absolute indices in an image to each led based on the border
		/// definition given in the list of leds. The map holds absolute indices to any given image,
		/// provided that it is row-oriented.
		/// The mapping is created purely on size (width and height). The given borders are excluded
		/// from indexing.
		///
		/// @param[in] width            The width of the indexed image
		/// @param[in] height           The width of the indexed image
		/// @param[in] horizontalBorder The size of the horizontal border (0=no border)
		/// @param[in] verticalBorder   The size of the vertical border (0=no border)
		/// @param[in] leds             The list with led specifications
		///
		ImageToLedsMap(
				const unsigned width,
				const unsigned height,
				const unsigned horizontalBorder,
				const unsigned verticalBorder,
				const std::vector<Led> & leds);

		///
		/// Returns the width of the indexed image
		///
		/// @return The width of the indexed image [pixels]
		///
		unsigned width() const;

		///
		/// Returns the height of the indexed image
		///
		/// @return The height of the indexed image [pixels]
		///
		unsigned height() const;

		///
		/// Determines the mean-color for each led using the mapping the image given
		/// at construction.
		///
		/// @param[in] image  The image from which to extract the led colors
		///
		/// @return ledColors  The vector containing the output
		///
		std::vector<RgbColor> getMeanLedColor(const RgbImage & image) const;

		///
		/// Determines the mean color for each led using the mapping the image given
		/// at construction.
		///
		/// @param[in] image  The image from which to extract the led colors
		/// @param[out] ledColors  The vector containing the output
		///
		void getMeanLedColor(const RgbImage & image, std::vector<RgbColor> & ledColors) const;

	private:
		/// The width of the indexed image
		const unsigned _width;
		/// The height of the indexed image
		const unsigned _height;
		/// The absolute indices into the image for each led
		std::vector<std::vector<unsigned>> mColorsMap;

		///
		/// Calculates the 'mean color' of the given list. This is the mean over each color-channel
		/// (red, green, blue)
		///
		/// @param[in] image The image a section from which an average color must be computed
		/// @param[in] colors  The list with colors
		///
		/// @return The mean of the given list of colors (or black when empty)
		///
		RgbColor calcMeanColor(const RgbImage & image, const std::vector<unsigned> & colors) const;
	};

} // end namespace hyperion
