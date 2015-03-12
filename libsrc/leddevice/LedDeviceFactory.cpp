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
	#include "LedDeviceAPA102.h"
#endif

#ifdef ENABLE_TINKERFORGE
	#include "LedDeviceTinkerforge.h"
#endif

#include "LedDeviceAdalight.h"
#include "LedDeviceAmbiLed.h"
#include "LedDeviceLightpack.h"
#include "LedDeviceMultiLightpack.h"
#include "LedDevicePaintpack.h"
#include "LedDevicePiBlaster.h"
#include "LedDeviceSedu.h"
#include "LedDeviceTest.h"
#include "LedDeviceHyperionUsbasp.h"
#include "LedDevicePhilipsHue.h"
#include "LedDeviceTpm2.h"
#include "LedDeviceAtmo.h"

#ifdef ENABLE_WS2812BPWM
	#include "LedDeviceWS2812b.h"
#endif

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
	else if (type == "ambiled")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();
		const int delay_ms       = deviceConfig["delayAfterConnect"].asInt();

		LedDeviceAmbiLed* deviceAmbiLed = new LedDeviceAmbiLed(output, rate, delay_ms);
		deviceAmbiLed->open();

		device = deviceAmbiLed;
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
	else if (type == "apa102")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceAPA102* deviceAPA102 = new LedDeviceAPA102(output, rate);
		deviceAPA102->open();

		device = deviceAPA102;
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
		const std::string username = deviceConfig.get("username", "newdeveloper").asString();
		const bool switchOffOnBlack = deviceConfig.get("switchOffOnBlack", true).asBool();
		const int transitiontime = deviceConfig.get("transitiontime", 1).asInt();
		std::vector<unsigned int> lightIds;
		for (Json::Value::ArrayIndex i = 0; i < deviceConfig["lightIds"].size(); i++) {
			lightIds.push_back(deviceConfig["lightIds"][i].asInt());
		}
		device = new LedDevicePhilipsHue(output, username, switchOffOnBlack, transitiontime, lightIds);
	}
	else if (type == "test")
	{
		const std::string output = deviceConfig["output"].asString();
		device = new LedDeviceTest(output);
	}
	else if (type == "tpm2")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate = deviceConfig["rate"].asInt();

		LedDeviceTpm2 * deviceTpm2 = new LedDeviceTpm2(output, rate);
		deviceTpm2->open();
		device = deviceTpm2;
	}
	else if (type == "atmo")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate = 38400;

		LedDeviceAtmo * deviceAtmo = new LedDeviceAtmo(output, rate);
		deviceAtmo->open();
		device = deviceAtmo;
	}
#ifdef ENABLE_WS2812BPWM
	else if (type == "ws2812b")
	{
		LedDeviceWS2812b * ledDeviceWS2812b = new LedDeviceWS2812b();
		device = ledDeviceWS2812b;
	}
#endif
	else
	{
		std::cout << "Unable to create device " << type << std::endl;
		// Unknown / Unimplemented device
	}
	return device;
}
