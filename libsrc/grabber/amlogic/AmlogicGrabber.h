#pragma once

// STL includes
#include <cstdint>

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgba.h>
#include <utils/VideoMode.h>

///
/// The DispmanxFrameGrabber is used for creating snapshots of the display (screenshots) with a
/// downsized and scaled resolution.
///
class AmlogicGrabber
{
public:
	///
	/// Construct a AmlogicGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] width  The width of the captured screenshot
	/// @param[in] height The heigth of the captured screenshot
	///
	AmlogicGrabber(const unsigned width, const unsigned height);
	~AmlogicGrabber();

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(const VideoMode videoMode);

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	///
	void grabFrame(Image<ColorRgba> & image);

private:

	/// With of the captured snapshot [pixels]
	const unsigned _width;
	/// Height of the captured snapshot [pixels]
	const unsigned _height;
	
	/** The snapshot/capture device of the amlogic video chip */
	int _amlogicCaptureDev;
};
