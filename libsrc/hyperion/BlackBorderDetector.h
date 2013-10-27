
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
		/// Falg indicating if the border is unknown
		bool unknown;

		/// The size of the detected horizontal border
		int horizontalSize;

		/// The size of the detected vertical border
		int verticalSize;

		///
		/// Compares this BlackBorder to the given other BlackBorder
		///
		/// @param[in] other  The other BlackBorder
		///
		/// @return True if this is the same border as other
		///
		inline bool operator== (const BlackBorder& other) const
		{
			if (unknown)
			{
				return other.unknown;
			}

			return other.unknown==false && horizontalSize==other.horizontalSize && verticalSize==other.verticalSize;
		}
	};

	///
	/// The BlackBorderDetector performs detection of black-borders on a single image.
	/// The detector will search for the upper left corner of the picture in the frame.
	/// Based on detected black pixels it will give an estimate of the black-border.
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
