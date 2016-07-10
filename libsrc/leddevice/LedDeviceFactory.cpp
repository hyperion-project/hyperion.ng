// Stl includes
#include <string>
#include <sstream>
#include <algorithm>

// Build configuration
#include <HyperionConfig.h>

// Leddevice includes
#include <leddevice/LedDeviceFactory.h>
#include <utils/Logger.h>

// Local Leddevice includes
#ifdef ENABLE_SPIDEV
	#include "LedDeviceLpd6803.h"
	#include "LedDeviceLpd8806.h"
	#include "LedDeviceP9813.h"
	#include "LedDeviceWs2801.h"
	#include "LedDeviceWs2812SPI.h"
	#include "LedDeviceSk6812SPI.h"
	#include "LedDeviceAPA102.h"
#endif

#ifdef ENABLE_TINKERFORGE
	#include "LedDeviceTinkerforge.h"
#endif

#include "LedDeviceAdalight.h"
#include "LedDeviceRawHID.h"
#include "LedDeviceLightpack.h"
#include "LedDeviceMultiLightpack.h"
#include "LedDevicePaintpack.h"
#include "LedDevicePiBlaster.h"
#include "LedDeviceSedu.h"
#include "LedDeviceFile.h"
#include "LedDeviceFadeCandy.h"
#include "LedDeviceUdp.h"
#include "LedDeviceUdpRaw.h"
#include "LedDeviceHyperionUsbasp.h"
#include "LedDevicePhilipsHue.h"
#include "LedDeviceTpm2.h"
#include "LedDeviceAtmo.h"
#include "LedDeviceAdalightApa102.h"
#include "LedDeviceAtmoOrb.h"

#ifdef ENABLE_WS2812BPWM
	#include "LedDeviceWS2812b.h"
#endif

#ifdef ENABLE_WS281XPWM
	#include "LedDeviceWS281x.h"
#endif

LedDevice * LedDeviceFactory::construct(const Json::Value & deviceConfig)
{
	Logger * log = Logger::getInstance("LedDevice");
	std::stringstream ss;
	ss << deviceConfig;
	Info(log, "configuration: %s ", ss.str().c_str());

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
	else if (type == "adalightapa102")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();
		const int delay_ms       = deviceConfig["delayAfterConnect"].asInt();

		LedDeviceAdalightApa102* deviceAdalightApa102 = new LedDeviceAdalightApa102(output, rate, delay_ms);
		deviceAdalightApa102->open();

		device = deviceAdalightApa102;
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
		const unsigned latchtime      = deviceConfig.get("latchtime",500000).asInt();

		LedDeviceWs2801* deviceWs2801 = new LedDeviceWs2801(output, rate, latchtime);
		deviceWs2801->open();

		device = deviceWs2801;
	}
	else if (type == "ws2812spi")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig.get("rate",2857143).asInt();

		LedDeviceWs2812SPI* deviceWs2812SPI = new LedDeviceWs2812SPI(output, rate);
		deviceWs2812SPI->open();

		device = deviceWs2812SPI;
	}
	else if (type == "sk6812rgbw-spi")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig.get("rate",2857143).asInt();
		const std::string& whiteAlgorithm = deviceConfig.get("white_algorithm","").asString();

		LedDeviceSk6812SPI* deviceSk6812SPI = new LedDeviceSk6812SPI(output, rate, whiteAlgorithm);
		deviceSk6812SPI->open();

		device = deviceSk6812SPI;
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
	else if (type == "rawhid")
	{
		const int delay_ms        = deviceConfig["delayAfterConnect"].asInt();
		auto VendorIdString       = deviceConfig.get("VID", "0x2341").asString();
		auto ProductIdString      = deviceConfig.get("PID", "0x8036").asString();

		// Convert HEX values to integer
		auto VendorId = std::stoul(VendorIdString, nullptr, 16);
		auto ProductId = std::stoul(ProductIdString, nullptr, 16);

		LedDeviceRawHID* deviceHID = new LedDeviceRawHID(VendorId, ProductId, delay_ms);
		deviceHID->open();

		device = deviceHID;
	}
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
		const int delay_ms        = deviceConfig["delayAfterConnect"].asInt();
		auto VendorIdString       = deviceConfig.get("VID", "0x0EBF").asString();
		auto ProductIdString      = deviceConfig.get("PID", "0x0025").asString();

		// Convert HEX values to integer
		auto VendorId = std::stoul(VendorIdString, nullptr, 16);
		auto ProductId = std::stoul(ProductIdString, nullptr, 16);

		LedDevicePaintpack * devicePainLightpack = new LedDevicePaintpack(VendorId, ProductId, delay_ms);
		devicePainLightpack->open();

		device = devicePainLightpack;
	}
	else if (type == "piblaster")
	{
		const std::string output     = deviceConfig.get("output",     "").asString();
		const std::string assignment = deviceConfig.get("assignment", "").asString();
		const Json::Value gpioMapping = deviceConfig.get("gpiomap", Json::nullValue);

		if (assignment.length() > 0) {
			Error(log, "Piblaster: The piblaster configuration syntax has changed in this version.");
			exit(EXIT_FAILURE);
		}
		if (! gpioMapping.isNull() ) {
			LedDevicePiBlaster * devicePiBlaster = new LedDevicePiBlaster(output, gpioMapping);
			devicePiBlaster->open();

			device = devicePiBlaster;
		} else {
			Error(log, "Piblaster: no gpiomap defined.");
			exit(EXIT_FAILURE);
		}
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
	else if (type == "atmoorb")
	{
		const std::string output = deviceConfig["output"].asString();
		const bool useOrbSmoothing = deviceConfig.get("useOrbSmoothing", false).asBool();
		const int transitiontime = deviceConfig.get("transitiontime", 1).asInt();
		const int skipSmoothingDiff = deviceConfig.get("skipSmoothingDiff", 0).asInt();
		const int port = deviceConfig.get("port", 1).asInt();
		const int numLeds = deviceConfig.get("numLeds", 1).asInt();
		const std::string orbId = deviceConfig["orbIds"].asString();
		std::vector<unsigned int> orbIds;

		// If we find multiple Orb ids separate them and add to list
		const std::string separator (",");
		if (orbId.find(separator) != std::string::npos) {
		  std::stringstream ss(orbId);
		  std::vector<int> output;
		  unsigned int i;
		  while (ss >> i) {
			  orbIds.push_back(i);
			  if (ss.peek() == ',' || ss.peek() == ' ')
				  ss.ignore();
		  }
		}
		else
		{
		  orbIds.push_back(atoi(orbId.c_str()));
		}

		device = new LedDeviceAtmoOrb(output, useOrbSmoothing, transitiontime, skipSmoothingDiff, port, numLeds, orbIds);
	}
	else if (type == "fadecandy")
	{
		const std::string host  = deviceConfig.get("output", "127.0.0.1").asString();
		const uint16_t port     = deviceConfig.get("port", 7890).asInt();
		const uint16_t channel  = deviceConfig.get("channel", 0).asInt();
		device = new LedDeviceFadeCandy(host, port, channel);
	}
	else if (type == "udp")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();
		const unsigned protocol  = deviceConfig["protocol"].asInt();
		const unsigned maxPacket   = deviceConfig["maxpacket"].asInt();
		device = new LedDeviceUdp(output, rate, protocol, maxPacket);
	}
	else if (type == "udpraw")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();
		const unsigned latchtime      = deviceConfig.get("latchtime",500000).asInt();

		LedDeviceUdpRaw* deviceUdpRaw = new LedDeviceUdpRaw(output, rate, latchtime);
		deviceUdpRaw->open();
		device = deviceUdpRaw;
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
#ifdef ENABLE_WS281XPWM
	else if (type == "ws281x")
	{
		const int gpio = deviceConfig.get("gpio", 18).asInt();
		const int leds = deviceConfig.get("leds", 256).asInt();
		const uint32_t freq = deviceConfig.get("freq", (Json::UInt)800000ul).asInt();
		const int dmanum = deviceConfig.get("dmanum", 5).asInt();
                const int pwmchannel = deviceConfig.get("pwmchannel", 0).asInt();
		const int invert = deviceConfig.get("invert", 0).asInt();
		const int rgbw = deviceConfig.get("rgbw", 0).asInt();
		const std::string& whiteAlgorithm = deviceConfig.get("white_algorithm","").asString();

		LedDeviceWS281x * ledDeviceWS281x = new LedDeviceWS281x(gpio, leds, freq, dmanum, pwmchannel, invert, 
			rgbw, whiteAlgorithm);
		device = ledDeviceWS281x;
	}
#endif
	else
	{
		if (type != "file")
		{
			Error(log, "Dummy device used, because unknown device %s set.", type.c_str());
		}
		const std::string output = deviceConfig.get("output", "/dev/null").asString();
		device = new LedDeviceFile(output);
	}

	return device;
}
