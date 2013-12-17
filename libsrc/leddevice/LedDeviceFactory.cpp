
// Leddevice includes
#include <leddevice/LedDeviceFactory.h>

// Local Leddevice includes
#include "LedDeviceLpd6803.h"
#include "LedDeviceLpd8806.h"
#include "LedDeviceSedu.h"
#include "LedDeviceTest.h"
#include "LedDeviceWs2801.h"
#include "LedDeviceWs2811.h"
#include "LedDeviceAdalight.h"
#include "LedDevicePaintpack.h"
#include "LedDeviceLightpack.h"
#include "LedDeviceMultiLightpack.h"

LedDevice * LedDeviceFactory::construct(const Json::Value & deviceConfig)
{
	std::cout << "Device configuration: " << deviceConfig << std::endl;

	std::string type = deviceConfig.get("type", "UNSPECIFIED").asString();
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);

	LedDevice* device = nullptr;
	if (type == "ws2801" || type == "lightberry")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceWs2801* deviceWs2801 = new LedDeviceWs2801(output, rate);
		deviceWs2801->open();

		device = deviceWs2801;
	}
	else if (type == "ws2811")
	{
		const std::string output       = deviceConfig["output"].asString();
		const std::string outputSpeed  = deviceConfig["output"].asString();
		const std::string timingOption = deviceConfig["timingOption"].asString();

		ws2811::SpeedMode speedMode = (outputSpeed == "high")? ws2811::highspeed : ws2811::lowspeed;
		if (outputSpeed != "high" && outputSpeed != "low")
		{
			std::cerr << "Incorrect speed-mode selected for WS2811: " << outputSpeed << " != {'high', 'low'}" << std::endl;
		}

		LedDeviceWs2811 * deviceWs2811 = new LedDeviceWs2811(output, ws2811::fromString(timingOption, ws2811::option_2855), speedMode);
		deviceWs2811->open();

		device = deviceWs2811;
	}
	else if (type == "lpd6803" || type == "ldp6803")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceLpd6803* deviceLdp6803 = new LedDeviceLpd6803(output, rate);
		deviceLdp6803->open();

		device = deviceLdp6803;
	}
	else if (type == "lpd8806" || type == "ldp8806")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceLpd8806* deviceLpd8806 = new LedDeviceLpd8806(output, rate);
		deviceLpd8806->open();

		device = deviceLpd8806;
	}
	else if (type == "sedu")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceSedu* deviceSedu = new LedDeviceSedu(output, rate);
		deviceSedu->open();

		device = deviceSedu;
	}
	else if (type == "adalight")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceAdalight* deviceAdalight = new LedDeviceAdalight(output, rate);
		deviceAdalight->open();

		device = deviceAdalight;
	}
	else if (type == "lightpack")
	{
		const std::string output = deviceConfig.get("output", "").asString();

		LedDeviceLightpack* deviceLightpack = new LedDeviceLightpack();
		deviceLightpack->open(output);

		device = deviceLightpack;
	}
	else if (type == "paintpack")
	{
		LedDevicePaintpack * devicePainLightpack = new LedDevicePaintpack();
		devicePainLightpack->open();

		device = devicePainLightpack;
	}
	else if (type == "multi-lightpack")
	{
		LedDeviceMultiLightpack* deviceLightpack = new LedDeviceMultiLightpack();
		deviceLightpack->open();

		device = deviceLightpack;
	}
	else if (type == "test")
	{
		const std::string output = deviceConfig["output"].asString();
		device = new LedDeviceTest(output);
	}
	else
	{
		std::cout << "Unable to create device " << type << std::endl;
		// Unknown / Unimplemented device
	}
	return device;
}
