#pragma once

// OSX includes
#include <CoreGraphics/CoreGraphics.h>

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/ImageResampler.h>

///
/// The OsxFrameGrabber is used for creating snapshots of the display (screenshots) 
///
class OsxFrameGrabber
{
public:
	///
	/// Construct a OsxFrameGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] display The index of the display to capture
	/// @param[in] width  The width of the captured screenshot
	/// @param[in] height The heigth of the captured screenshot
	///
	OsxFrameGrabber(const unsigned display, const unsigned width, const unsigned height);
	~OsxFrameGrabber();

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
	void grabFrame(Image<ColorRgb> & image);

private:	
	/// display
	const unsigned _screenIndex;
	
	/// With of the captured snapshot [pixels]
	const unsigned _width;
	
	/// Height of the captured snapshot [pixels]
	const unsigned _height;
	
	/// Reference to the captured diaplay
	CGDirectDisplayID _display;
	
	/// Image resampler for downscaling the image
	ImageResampler * _imgResampler;
};
