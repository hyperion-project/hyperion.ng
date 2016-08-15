#pragma once

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <utils/ImageResampler.h>
#include <utils/Logger.h>

///
/// The FramebufferFrameGrabber is used for creating snapshots of the display (screenshots) 
///
class FramebufferFrameGrabber
{
public:
	///
	/// Construct a FramebufferFrameGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] device The framebuffer device name/path
	/// @param[in] width  The width of the captured screenshot
	/// @param[in] height The heigth of the captured screenshot
	///
	FramebufferFrameGrabber(const std::string & device, const unsigned width, const unsigned height);
	~FramebufferFrameGrabber();

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
	/// Framebuffer file descriptor
	int _fbfd;

	/// Pointer to framebuffer
	unsigned char * _fbp;
	
	/// Framebuffer device e.g. /dev/fb0
	const std::string _fbDevice;
	
	/// With of the captured snapshot [pixels]
	const unsigned _width;
	
	/// Height of the captured snapshot [pixels]
	const unsigned _height;

	/// Image resampler for downscaling the image
	ImageResampler * _imgResampler;
	
	Logger * _log;
};
