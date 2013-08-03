#pragma once

// VC includes
#include <bcm_host.h>

template <typename Hyperion_T>
int dispmanx_process(Hyperion_T& hyperion, volatile bool& running)
{
	// Configure the used image size
	const unsigned width  = 64;
	const unsigned height = 64;
	hyperion.setInputSize(width, height);

	// Initiase BCM
	bcm_host_init();

	// Open the connection to the displaydisplay
	DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);
	DISPMANX_MODEINFO_T info;
	int ret = vc_dispmanx_display_get_info(display, &info);
	assert(ret == 0);

	// Create the resources for capturing image
	uint32_t vc_image_ptr;
	DISPMANX_RESOURCE_HANDLE_T resource = vc_dispmanx_resource_create(
			VC_IMAGE_RGB888,
			width,
			height,
			&vc_image_ptr);
	assert(resource);

	VC_RECT_T rectangle;
	vc_dispmanx_rect_set(&rectangle, 0, 0, width, height);

	void* image_ptr = hyperion.image().memptr();
	const uint32_t pitch = width * 3;

	timespec updateInterval;
	updateInterval.tv_sec  = 0;
	updateInterval.tv_nsec = 100000000;
	while(running)
	{
		vc_dispmanx_snapshot(display, resource, VC_IMAGE_ROT0);
		vc_dispmanx_resource_read_data(resource, &rectangle, image_ptr, pitch);

		hyperion.commit();

		nanosleep(&updateInterval, NULL);
	}

	// Clean up resources
	vc_dispmanx_resource_delete(resource);
	vc_dispmanx_display_close(display);

	// De-init BCM
	bcm_host_deinit();

	return 0;
}
