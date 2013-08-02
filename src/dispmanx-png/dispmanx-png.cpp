
// STL includes
#include <csignal>

// VC includes
#include <bcm_host.h>

// Hyperion includes
#include <hyperionpng/HyperionPng.h>

static volatile bool sRunning = true;

void signal_handler(int signum)
{
	std::cout << "RECEIVED SIGNAL: " << signum << std::endl;
	sRunning = false;
}

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

	RgbImage* image_ptr = &(hyperion.image());
	void* image_vp = reinterpret_cast<void*>(image_ptr);
	const uint32_t pitch = width * 3;

	timespec updateInterval;
	updateInterval.tv_sec  = 0;
	updateInterval.tv_nsec = 100000000;
	while(running)
	{
		std::cout << "Grabbing a frame from display" << std::endl;
		vc_dispmanx_snapshot(display, resource, VC_IMAGE_ROT0);
		vc_dispmanx_resource_read_data(resource, &rectangle, image_vp, pitch);

		std::cout << "Commiting the frame to Hyperion" << std::endl;
//		hyperion.commit();

		std::cout << "Waiting for next grab" << std::endl;
		nanosleep(&updateInterval, NULL);
	}

	std::cout << "Cleaning VC resources" << std::endl;
	// Clean up resources
	vc_dispmanx_resource_delete(resource);
	vc_dispmanx_display_close(display);

	std::cout << "Uninitialising BCM-Host" << std::endl;
	// De-init BCM
	bcm_host_deinit();

	std::cout << "Exit success" << std::endl;

	return 0;
}

int main(int /*argc*/, char** /*argv*/)
{
	// Install signal-handlers to exit the processing loop
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);

	// Construct and initialise the PNG creator with preset size
	HyperionPng hyperion;
	return dispmanx_process(hyperion, sRunning);
}
