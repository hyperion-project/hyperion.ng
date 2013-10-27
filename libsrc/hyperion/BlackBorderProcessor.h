
#pragma once

// Local Hyperion includes
#include "BlackBorderDetector.h"

namespace hyperion
{
	///
	/// The BlackBorder processor is a wrapper around the black-border detector for keeping track of
	/// detected borders and count of the type and size of detected borders.
	///
	class BlackBorderProcessor
	{
	public:
		///
		/// Constructor for the BlackBorderProcessor
		/// @param unknownFrameCnt The number of frames(images) that need to contain an unknown
		///                        border before the current border is set to unknown
		/// @param borderFrameCnt The number of frames(images) that need to contain a vertical or
		///                       horizontal border becomes the current border
		/// @param blurRemoveCnt The size to add to a horizontal or vertical border (because the
		///                      outer pixels is blurred (black and color combined due to image scaling))
		///
		BlackBorderProcessor(
				const unsigned unknownFrameCnt,
				const unsigned borderFrameCnt,
				const unsigned blurRemoveCnt);

		///
		/// Return the current (detected) border
		/// @return The current border
		///
		BlackBorder getCurrentBorder() const;

		///
		/// Processes the image. This performs detecion of black-border on the given image and
		/// updates the current border accordingly. If the current border is updated the method call
		/// will return true else false
		///
		/// @param image The image to process
		///
		/// @return True if a different border was detected than the current else false
		///
		bool process(const RgbImage& image);

	private:

		/// The number of unknown-borders detected before it becomes the current border
		const unsigned _unknownSwitchCnt;

		/// The number of horizontal/vertical borders detected before it becomes the current border
		const unsigned _borderSwitchCnt;

		/// The number of pixels to increase a detected border for removing blury pixels
		unsigned _blurRemoveCnt;

		/// The blackborder detector
		BlackBorderDetector _detector;

		/// The current detected border
		BlackBorder _currentBorder;

		/// The border detected in the previous frame
		BlackBorder _previousDetectedBorder;

		/// The number of frame the previous detected border matched the incomming border
		unsigned _consistentCnt;
	};
} // end namespace hyperion
