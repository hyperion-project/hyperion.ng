// Stl includes
#include <string>
#include <sstream>
#include <algorithm>
#include <exception>
#include <map>

// Build configuration
#include <HyperionConfig.h>

// Leddevice includes
#include <leddevice/LedDeviceFactory.h>
#include <utils/Logger.h>
#include <leddevice/LedDevice.h>

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
#include "LedDeviceUdpE131.h"
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

	// rs232 devices
	LedDevice::addToDeviceMap("adalight"      , LedDeviceAdalight::construct);
	LedDevice::addToDeviceMap("adalightapa102", LedDeviceAdalightApa102::construct);
	LedDevice::addToDeviceMap("sedu"          , LedDeviceSedu::construct);
	LedDevice::addToDeviceMap("tpm2"          , LedDeviceTpm2::construct);
	LedDevice::addToDeviceMap("atmo"          , LedDeviceAtmo::construct);
	LedDevice::addToDeviceMap("fadecandy"     , LedDeviceFadeCandy::construct);

	// spi devices
	#ifdef ENABLE_SPIDEV
	LedDevice::addToDeviceMap("apa102"        , LedDeviceAPA102::construct);
	LedDevice::addToDeviceMap("lpd6803"       , LedDeviceLpd6803::construct);
	LedDevice::addToDeviceMap("lpd8806"       , LedDeviceLpd8806::construct);
	LedDevice::addToDeviceMap("p9813"         , LedDeviceP9813::construct);
	LedDevice::addToDeviceMap("ws2801"        , LedDeviceWs2801::construct);
	LedDevice::addToDeviceMap("ws2812spi"     , LedDeviceWs2812SPI::construct);
	LedDevice::addToDeviceMap("sk6812rgbw-spi", LedDeviceSk6812SPI::construct);
	#endif
	
	// pwm devices
	#ifdef ENABLE_WS2812BPWM
	LedDevice::addToDeviceMap("ws2812b", LedDeviceWS2812b::construct);
	#endif

	// other
	LedDevice::addToDeviceMap("file", LedDeviceFile::construct);
	
	// network lights
	#ifdef ENABLE_TINKERFORGE
	LedDevice::addToDeviceMap("tinkerforge", LedDeviceTinkerforge::construct);
	#endif
	
	const LedDeviceRegistry& devList = LedDevice::getDeviceMap();
	LedDevice* device = nullptr;
	try
	{
		for ( auto dev: devList)
		{
			if (dev.first == type)
			{
				device = dev.second(deviceConfig);
				Info(log,"LedDevice '%s' configured.", dev.first.c_str());
				break;
			}
		}
	
	// ===== old config =====
		if (device != nullptr) { /* do nothing */ }
		else if (type == "rawhid")
		{
			const int delay_ms        = deviceConfig["delayAfterConnect"].asInt();
			auto VendorIdString       = deviceConfig.get("VID", "0x2341").asString();
			auto ProductIdString      = deviceConfig.get("PID", "0x8036").asString();

			// Convert HEX values to integer
			auto VendorId = std::stoul(VendorIdString, nullptr, 16);
			auto ProductId = std::stoul(ProductIdString, nullptr, 16);

			device = new LedDeviceRawHID(VendorId, ProductId, delay_ms);
		}
		else if (type == "lightpack")
		{
			device = new LedDeviceLightpack(deviceConfig.get("output", "").asString());
		}
		else if (type == "multi-lightpack")
		{
			device = new LedDeviceMultiLightpack();
		}
		else if (type == "paintpack")
		{
			const int delay_ms        = deviceConfig["delayAfterConnect"].asInt();
			auto VendorIdString       = deviceConfig.get("VID", "0x0EBF").asString();
			auto ProductIdString      = deviceConfig.get("PID", "0x0025").asString();

			// Convert HEX values to integer
			auto VendorId = std::stoul(VendorIdString, nullptr, 16);
			auto ProductId = std::stoul(ProductIdString, nullptr, 16);

			device = new LedDevicePaintpack(VendorId, ProductId, delay_ms);
		}
		else if (type == "piblaster")
		{
			const std::string output     = deviceConfig.get("output",     "").asString();
			const std::string assignment = deviceConfig.get("assignment", "").asString();
			const Json::Value gpioMapping = deviceConfig.get("gpiomap", Json::nullValue);

			if (! assignment.empty())
			{
				throw std::runtime_error("Piblaster: The piblaster configuration syntax has changed in this version.");
			}
			if (gpioMapping.isNull())
			{
				throw std::runtime_error("Piblaster: no gpiomap defined.");
			}
			device = new LedDevicePiBlaster(output, gpioMapping);
		}
		else if (type == "hyperion-usbasp-ws2801")
		{
			device = new LedDeviceHyperionUsbasp(LedDeviceHyperionUsbasp::CMD_WRITE_WS2801);
		}
		else if (type == "hyperion-usbasp-ws2812")
		{
			device = new LedDeviceHyperionUsbasp(LedDeviceHyperionUsbasp::CMD_WRITE_WS2812);
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
		else if (type == "udp")
		{
			device = new LedDeviceUdp(
				deviceConfig["output"].asString(),
				deviceConfig["protocol"].asInt(),
				deviceConfig["maxpacket"].asInt()
			);
		}
		else if (type == "udpraw")
		{
			device = new LedDeviceUdpRaw(
				deviceConfig["output"].asString(),
				deviceConfig.get("latchtime",500000).asInt()
			);
		}
		else if (type == "e131")
		{
			device = new LedDeviceUdpE131(
				deviceConfig["output"].asString(),
				deviceConfig.get("latchtime",104000).asInt(),
				deviceConfig.get("universe",1).asInt()
			);
		}

	#ifdef ENABLE_WS281XPWM
		else if (type == "ws281x")
		{
			device = new LedDeviceWS281x(
				deviceConfig.get("gpio", 18).asInt(),
				deviceConfig.get("leds", 256).asInt(),
				deviceConfig.get("freq", (Json::UInt)800000ul).asInt(),
				deviceConfig.get("dmanum", 5).asInt(),
				deviceConfig.get("pwmchannel", 0).asInt(),
				deviceConfig.get("invert", 0).asInt(),
				deviceConfig.get("rgbw", 0).asInt(),
				deviceConfig.get("white_algorithm","").asString()
			);
		}
	#endif
		else
		{
			Error(log, "Dummy device used, because configured device '%s' is unknown", type.c_str() );
			throw std::runtime_error("unknown device");
		}
	}
	catch(std::exception& e)
	{
		
		Error(log, "Dummy device used, because configured device '%s' throws error '%s'", type.c_str(), e.what());
		const Json::Value dummyDeviceConfig;
		device = LedDeviceFile::construct(dummyDeviceConfig);
	}

	device->open();
	
	return device;
}
