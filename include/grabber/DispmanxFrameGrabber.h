#pragma once

// BCM includes
#pragma GCC system_header
#include <bcm_host.h>

// Utils includes
#include <utils/ColorRgba.h>
#include <hyperion/Grabber.h>

///
/// The DispmanxFrameGrabber is used for creating snapshots of the display (screenshots) with a
/// downsized and scaled resolution.
///
class DispmanxFrameGrabber : public Grabber
{
public:
	///
	/// Construct a DispmanxFrameGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] width  The width of the captured screenshot
	/// @param[in] height The heigth of the captured screenshot
	///
	DispmanxFrameGrabber(const unsigned width, const unsigned height);
	~DispmanxFrameGrabber();

	///
	/// Updates the frame-grab flags as used by the VC library for frame grabbing
	///
	/// @param vc_flags  The snapshot grabbing mask
	///
	void setFlags(const int vc_flags);

	void setCropping(const unsigned cropLeft, const unsigned cropRight,
			 const unsigned cropTop, const unsigned cropBottom);

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
	/// Handle to the display that is being captured
	DISPMANX_DISPLAY_HANDLE_T _vc_display;

	/// Handle to the resource for storing the captured snapshot
	DISPMANX_RESOURCE_HANDLE_T _vc_resource;

	/// Rectangle of the captured resource that is transfered to user space
	VC_RECT_T _rectangle;

	/// Flags (transforms) for creating snapshots
	int _vc_flags;

	// temp buffer when capturing with unsupported pitch size or
	// when we need to crop the image
	ColorRgba* _captureBuffer;

	// size of the capture buffer in Pixels
	unsigned _captureBufferSize;
};
