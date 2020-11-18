#pragma once

// BCM includes
#ifdef PLATFORM_RPI
	#pragma GCC system_header
	#include <bcm_host.h>
#else
	#include <grabber/DispmanxFrameGrabberMock.h>
#endif

// Utils includes
#include <utils/Image.h>
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
	DispmanxFrameGrabber(unsigned width, unsigned height);
	~DispmanxFrameGrabber() override;


	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	///
	int grabFrame(Image<ColorRgb> & image);

	///
	///@brief Set new width and height for dispmanx, overwrite Grabber.h impl
	bool setWidthHeight(int width, int height) override;

private:
	///
	/// Updates the frame-grab flags as used by the VC library for frame grabbing
	///
	/// @param vc_flags  The snapshot grabbing mask
	///
	void setFlags(int vc_flags);

	///
	/// @brief free _vc_resource and captureBuffer
	///
	void freeResources();

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

	// rgba output buffer
	Image<ColorRgba>  _image_rgba;

};
