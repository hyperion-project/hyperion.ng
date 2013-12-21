
// STL includes
#include <cassert>
#include <iostream>

// Local includes
#include "DispmanxFrameGrabber.h"

DispmanxFrameGrabber::DispmanxFrameGrabber(const unsigned width, const unsigned height) :
	_vc_display(0),
	_vc_resource(0),
	_vc_flags(0),
	_width(width),
	_height(height)
{
	// Initiase BCM
	bcm_host_init();

	{
		// Check if the display can be opened and display the current resolution
		// Open the connection to the display
		_vc_display = vc_dispmanx_display_open(0);
		assert(_vc_display > 0);

		// Obtain the display information
		DISPMANX_MODEINFO_T vc_info;
		int result = vc_dispmanx_display_get_info(_vc_display, &vc_info);
		// Keep compiler happy in 'release' mode
		(void)result;
		assert(result == 0);
		std::cout << "Display opened with resolution: " << vc_info.width << "x" << vc_info.height << std::endl;

		// Close the displaye
		vc_dispmanx_display_close(_vc_display);
	}

	// Create the resources for capturing image
	uint32_t vc_nativeImageHandle;
	_vc_resource = vc_dispmanx_resource_create(
			VC_IMAGE_RGBA32,
			width,
			height,
			&vc_nativeImageHandle);
	assert(_vc_resource);

	// Define the capture rectangle with the same size
	vc_dispmanx_rect_set(&_rectangle, 0, 0, width, height);
}

DispmanxFrameGrabber::~DispmanxFrameGrabber()
{
	// Clean up resources
	vc_dispmanx_resource_delete(_vc_resource);

	// De-init BCM
	bcm_host_deinit();
}

void DispmanxFrameGrabber::setFlags(const int vc_flags)
{
	_vc_flags = vc_flags;
}

void DispmanxFrameGrabber::setVideoMode(const VideoMode videoMode)
{
	switch (videoMode) {
	case VIDEO_3DSBS:
		vc_dispmanx_rect_set(&_rectangle, 0, 0, _width/2, _height);
		break;
	case VIDEO_3DTAB:
		vc_dispmanx_rect_set(&_rectangle, 0, 0, _width, _height/2);
		break;
	case VIDEO_2D:
	default:
		vc_dispmanx_rect_set(&_rectangle, 0, 0, _width, _height);
		break;
	}
}

void DispmanxFrameGrabber::grabFrame(Image<ColorRgba> & image)
{
	// resize the given image if needed
	if (image.width() != unsigned(_rectangle.width) || image.height() != unsigned(_rectangle.height))
	{
		image.resize(_rectangle.width, _rectangle.height);
	}

	// Open the connection to the display
	_vc_display = vc_dispmanx_display_open(0);

	// Create the snapshot (incl down-scaling)
	vc_dispmanx_snapshot(_vc_display, _vc_resource, (DISPMANX_TRANSFORM_T) _vc_flags);

	// Read the snapshot into the memory
	void* image_ptr = image.memptr();
	const unsigned destPitch = _rectangle.width * sizeof(ColorRgba);
	vc_dispmanx_resource_read_data(_vc_resource, &_rectangle, image_ptr, destPitch);

	// Close the displaye
	vc_dispmanx_display_close(_vc_display);
}
