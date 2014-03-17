#pragma once

// Utils includes
#include <utils/Image.h>

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
		/// @param[in] blackborderThreshold The threshold which the blackborder detector should use
		///
		BlackBorderDetector(uint8_t blackborderThreshold);

		///
		/// Performs the actual black-border detection on the given image
		///
		/// @param[in] image  The image on which detection is performed
		///
		/// @return The detected (or not detected) black border info
		///
		template <typename Pixel_T>
		BlackBorder process(const Image<Pixel_T> & image)
		{
			// only test the topleft third of the image
			int width = image.width() /3;
			int height = image.height() / 3;
			int maxSize = std::max(width, height);

			int firstNonBlackXPixelIndex = -1;
			int firstNonBlackYPixelIndex = -1;

			// find some pixel of the image
			for (int i = 0; i < maxSize; ++i)
			{
				int x = std::min(i, width);
				int y = std::min(i, height);

				const Pixel_T & color = image(x, y);
				if (!isBlack(color))
				{
					firstNonBlackXPixelIndex = x;
					firstNonBlackYPixelIndex = y;
					break;
				}
			}

			// expand image to the left
			for(; firstNonBlackXPixelIndex > 0; --firstNonBlackXPixelIndex)
			{
				const Pixel_T & color = image(firstNonBlackXPixelIndex-1, firstNonBlackYPixelIndex);
				if (isBlack(color))
				{
					break;
				}
			}

			// expand image to the top
			for(; firstNonBlackYPixelIndex > 0; --firstNonBlackYPixelIndex)
			{
				const Pixel_T & color = image(firstNonBlackXPixelIndex, firstNonBlackYPixelIndex-1);
				if (isBlack(color))
				{
					break;
				}
			}

			// Construct result
			BlackBorder detectedBorder;
			detectedBorder.unknown = firstNonBlackXPixelIndex == -1 || firstNonBlackYPixelIndex == -1;
			detectedBorder.horizontalSize = firstNonBlackYPixelIndex;
			detectedBorder.verticalSize = firstNonBlackXPixelIndex;
			return detectedBorder;
		}

	private:

		///
		/// Checks if a given color is considered black and therefor could be part of the border.
		///
		/// @param[in] color  The color to check
		///
		/// @return True if the color is considered black else false
		///
		template <typename Pixel_T>
		inline bool isBlack(const Pixel_T & color)
		{
			// Return the simple compare of the color against black
			return color.red < _blackborderThreshold && color.green < _blackborderThreshold && color.blue < _blackborderThreshold;
		}

	private:
		/// Threshold for the blackborder detector [0 .. 255]
		const uint8_t _blackborderThreshold;
	};
} // end namespace hyperion
