
// VC includes
#include <bcm_host.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

#include <json/json.h>
#include <utils/jsonschema/JsonFactory.h>

int main(int /*argc*/, char** /*argv*/)
{
	const char* homeDir = getenv("RASPILIGHT_HOME");
	if (!homeDir)
	{
		homeDir = "/etc";
	}
	std::cout << "RASPILIGHT HOME DIR: " << homeDir << std::endl;

	const std::string schemaFile = std::string(homeDir) + "/hyperion.schema.json";
	const std::string configFile = std::string(homeDir) + "/hyperion.config.json";

	Json::Value raspiConfig;
	if (JsonFactory::load(schemaFile, configFile, raspiConfig) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << std::endl;
		return -1;
	}
	Hyperion hyperion(raspiConfig);

	const unsigned width  = 64;
	const unsigned height = 64;

	hyperion.setInputSize(width, height);

	volatile bool running = true;

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
	const unsigned imageSize_bytes = width*height*3;

	timespec updateInterval;
	updateInterval.tv_sec  = 0;
	updateInterval.tv_nsec = 100000000000;

	while(running)
	{
		vc_dispmanx_snapshot(display, resource, VC_IMAGE_ROT0);
		vc_dispmanx_resource_read_data(resource, &rectangle, image_vp, imageSize_bytes);

		hyperion.commit();

		nanosleep(&updateInterval, NULL);
	}

	return 0;
}
