
#include "DispmanxFrameGrabber.h"

DispmanxFrameGrabber::DispmanxFrameGrabber(const unsigned width, const unsigned height) :
	_display(0),
	_resource(0),
	_width(width),
	_height(height)
{
	// Initiase BCM
	bcm_host_init();

	// Open the connection to the displaydisplay
	_display = vc_dispmanx_display_open(0);
	int ret = vc_dispmanx_display_get_info(_display, &_info);
	assert(ret == 0);

	// Create the resources for capturing image
	_resource = vc_dispmanx_resource_create(
			VC_IMAGE_RGB888,
			width,
			height,
			&_vc_image_ptr);
	assert(_resource);

	// Define the capture rectangle with the same size
	vc_dispmanx_rect_set(&_rectangle, 0, 0, width, height);
	_pitch = width * 3;
}

DispmanxFrameGrabber::~DispmanxFrameGrabber()
{
	// Clean up resources
	vc_dispmanx_resource_delete(_resource);

	// Close the displaye
	vc_dispmanx_display_close(_display);

	// De-init BCM
	bcm_host_deinit();
}

void DispmanxFrameGrabber::grabFrame(RgbImage& image)
{
	// Sanity check of the given image size
	assert(image.width() == _width && image.height() == _height);

	void* image_ptr = image.memptr();

	// Create the snapshot
	vc_dispmanx_snapshot(_display, _resource, VC_IMAGE_ROT0);
	// Read the snapshot into the memory (incl down-scaling)
	vc_dispmanx_resource_read_data(_resource, &_rectangle, image_ptr, _pitch);
}
