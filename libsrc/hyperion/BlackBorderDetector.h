
#pragma once

// Utils includes
#include <utils/RgbImage.h>

namespace hyperion
{
	///
	/// Result structure of the detected blackborder.
	///
	struct BlackBorder
	{
		///
		/// Enumeration of the possible types of detected borders
		///
		enum Type
		{
			none,
			horizontal,
			vertical,
			unknown
		};

		/// The type of detected border
		Type type;

		/// The size of detected border (negative if not applicable)
		int size;
	};

	///
	/// The BlackBorderDetector performs detection of black-borders on a single image.
	/// The detector will scan the border of the upper-left quadrant of an image. Based on detected
	/// black pixels it will give an estimate of the black-border.
	///
	class BlackBorderDetector
	{
	public:
		///
		/// Constructs a black-border detector
		///
		BlackBorderDetector();

		///
		/// Performs the actual black-border detection on the given image
		///
		/// @param[in] image  The image on which detection is performed
		///
		/// @return The detected (or not detected) black border info
		///
		BlackBorder process(const RgbImage& image);

	private:

		///
		/// Checks if a given color is considered black and therefor could be part of the border.
		///
		/// @param[in] color  The color to check
		///
		/// @return True if the color is considered black else false
		///
		inline bool isBlack(const RgbColor& color)
		{
			// Return the simple compare of the color against black
			return RgbColor::BLACK == color;
			// TODO[TvdZ]: We could add a threshold to check that the color is close to black
		}
	};
} // end namespace hyperion
