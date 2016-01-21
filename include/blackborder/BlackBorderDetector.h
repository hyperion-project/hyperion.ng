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

			// test center and 33%, 66% of width/heigth
			// 33 and 66 will check left and top
			// center ill check right and bottom sids
			int width = image.width();
			int height = image.height();
			int width33percent = width / 3;
			int height33percent = height / 3;
			int width66percent = width33percent * 2;
			int height66percent = height33percent * 2;
			int xCenter = width / 2;
			int yCenter = height / 2;


			int firstNonBlackXPixelIndex = -1;
			int firstNonBlackYPixelIndex = -1;

			// find first X pixel of the image
			for (int x = 0; x < width; ++x)
			{
				const Pixel_T & color1 = image( (width - x), yCenter); // right side center line check
				const Pixel_T & color2 = image(x, height33percent);
				const Pixel_T & color3 = image(x, height66percent);
				if (!isBlack(color1) || !isBlack(color2) || !isBlack(color3))
				{
					firstNonBlackXPixelIndex = x;
					break;
				}
			}

			// find first Y pixel of the image
			for (int y = 0; y < height; ++y)
			{
				const Pixel_T & color1 = image(xCenter, (height - y)); // bottom center line check
				const Pixel_T & color2 = image(width33percent, y );
				const Pixel_T & color3 = image(width66percent, y);
				if (!isBlack(color1) || !isBlack(color2) || !isBlack(color3))
				{
					firstNonBlackYPixelIndex = y;
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
