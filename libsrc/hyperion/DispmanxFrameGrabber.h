#pragma once

// BCM includes
#include <bcm_host.h>

// STL includes
#include <cstdint>

// Utils includes
#include <utils/RgbImage.h>

///
/// The DispmanxFrameGrabber grabs
///
class DispmanxFrameGrabber
{
public:
	DispmanxFrameGrabber(const unsigned width, const unsigned height);
	~DispmanxFrameGrabber();

	void grabFrame(RgbImage& image);

private:
	DISPMANX_DISPLAY_HANDLE_T _display;
	DISPMANX_MODEINFO_T _info;

	DISPMANX_RESOURCE_HANDLE_T _resource;

	uint32_t _vc_image_ptr;

	VC_RECT_T _rectangle;
	unsigned _width;
	unsigned _height;
	uint32_t _pitch;
};
