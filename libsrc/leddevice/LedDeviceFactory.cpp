// Stl includes
#include <string>
#include <algorithm>

// Build configuration
#include <HyperionConfig.h>

// Leddevice includes
#include <leddevice/LedDeviceFactory.h>

// Local Leddevice includes
#ifdef ENABLE_SPIDEV
	#include "LedDeviceLpd6803.h"
	#include "LedDeviceLpd8806.h"
	#include "LedDeviceP9813.h"
	#include "LedDeviceWs2801.h"
#endif

#ifdef ENABLE_TINKERFORGE
	#include "LedDeviceTinkerforge.h"
#endif

#include "LedDeviceAdalight.h"
#include "LedDeviceLightpack.h"
#include "LedDeviceMultiLightpack.h"
#include "LedDevicePaintpack.h"
#include "LedDevicePiBlaster.h"
#include "LedDeviceSedu.h"
#include "LedDeviceTest.h"
#include "LedDeviceHyperionUsbasp.h"
#include "LedDevicePhilipsHue.h"

LedDevice * LedDeviceFactory::construct(const Json::Value & deviceConfig)
{
	std::cout << "Device configuration: " << deviceConfig << std::endl;

	std::string type = deviceConfig.get("type", "UNSPECIFIED").asString();
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);

	LedDevice* device = nullptr;
	if (false) {}
	else if (type == "adalight")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();
		const int delay_ms       = deviceConfig["delayAfterConnect"].asInt();

		LedDeviceAdalight* deviceAdalight = new LedDeviceAdalight(output, rate, delay_ms);
		deviceAdalight->open();

		device = deviceAdalight;
	}
#ifdef ENABLE_SPIDEV
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
	else if (type == "p9813")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceP9813* deviceP9813 = new LedDeviceP9813(output, rate);
		deviceP9813->open();

		device = deviceP9813;
	}
	else if (type == "ws2801" || type == "lightberry")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceWs2801* deviceWs2801 = new LedDeviceWs2801(output, rate);
		deviceWs2801->open();

		device = deviceWs2801;
	}
#endif
#ifdef ENABLE_TINKERFORGE
	else if (type=="tinkerforge")
	{
		const std::string host 	= deviceConfig.get("output", "127.0.0.1").asString();
		const uint16_t port 		= deviceConfig.get("port", 4223).asInt();
		const std::string  uid		= deviceConfig["uid"].asString();
		const unsigned rate 	= deviceConfig["rate"].asInt();

		LedDeviceTinkerforge* deviceTinkerforge = new LedDeviceTinkerforge(host, port, uid, rate);
		deviceTinkerforge->open();

		device = deviceTinkerforge;
	}
#endif
	else if (type == "lightpack")
	{
		const std::string output = deviceConfig.get("output", "").asString();

		LedDeviceLightpack* deviceLightpack = new LedDeviceLightpack();
		deviceLightpack->open(output);

		device = deviceLightpack;
	}
	else if (type == "multi-lightpack")
	{
		LedDeviceMultiLightpack* deviceLightpack = new LedDeviceMultiLightpack();
		deviceLightpack->open();

		device = deviceLightpack;
	}
	else if (type == "paintpack")
	{
		LedDevicePaintpack * devicePainLightpack = new LedDevicePaintpack();
		devicePainLightpack->open();

		device = devicePainLightpack;
	}
	else if (type == "piblaster")
	{
		const std::string output     = deviceConfig.get("output",     "").asString();
		const std::string assignment = deviceConfig.get("assignment", "").asString();

		LedDevicePiBlaster * devicePiBlaster = new LedDevicePiBlaster(output, assignment);
		devicePiBlaster->open();

		device = devicePiBlaster;
	}
	else if (type == "sedu")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceSedu* deviceSedu = new LedDeviceSedu(output, rate);
		deviceSedu->open();

		device = deviceSedu;
	}
	else if (type == "hyperion-usbasp-ws2801")
	{
			LedDeviceHyperionUsbasp * deviceHyperionUsbasp = new LedDeviceHyperionUsbasp(LedDeviceHyperionUsbasp::CMD_WRITE_WS2801);
			deviceHyperionUsbasp->open();
			device = deviceHyperionUsbasp;
	}
	else if (type == "hyperion-usbasp-ws2812")
	{
			LedDeviceHyperionUsbasp * deviceHyperionUsbasp = new LedDeviceHyperionUsbasp(LedDeviceHyperionUsbasp::CMD_WRITE_WS2812);
			deviceHyperionUsbasp->open();
			device = deviceHyperionUsbasp;
	}
	else if (type == "philipshue")
	{
		const std::string output = deviceConfig["output"].asString();
		const bool switchOffOnBlack = deviceConfig.get("switch_off_on_black", false).asBool();
		device = new LedDevicePhilipsHue(output, switchOffOnBlack);
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
