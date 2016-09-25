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
#include "LedDeviceTpm2net.h"
#include "LedDeviceUdpRaw.h"
#include "LedDeviceUdpE131.h"
#include "LedDeviceHyperionUsbasp.h"
#include "LedDevicePhilipsHue.h"
#include "LedDeviceTpm2.h"
#include "LedDeviceAtmo.h"
#include "LedDeviceAdalightApa102.h"
#include "LedDeviceAtmoOrb.h"
#include "LedDeviceUdpH801.h"

#ifdef ENABLE_WS2812BPWM
	#include "LedDeviceWS2812b.h"
#endif

#ifdef ENABLE_WS281XPWM
	#include "LedDeviceWS281x.h"
#endif

LedDevice * LedDeviceFactory::construct(const Json::Value & deviceConfig, const int ledCount)
{
	Logger * log = Logger::getInstance("LedDevice");
	std::stringstream ss;
	ss << deviceConfig;
	Info(log, "configuration: %s ", ss.str().c_str());

	std::string type = deviceConfig.get("type", "UNSPECIFIED").asString();
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);

	// set amount of led to leddevice
	LedDevice::setLedCount(ledCount);

	#define REGISTER(devName,className) LedDevice::addToDeviceMap(devName, className::construct);
	// rs232 devices
	REGISTER("adalight"      , LedDeviceAdalight);
	REGISTER("adalightapa102", LedDeviceAdalightApa102);
	REGISTER("sedu"          , LedDeviceSedu);
	REGISTER("tpm2"          , LedDeviceTpm2);
	REGISTER("atmo"          , LedDeviceAtmo);

	// spi devices
	#ifdef ENABLE_SPIDEV
	REGISTER("apa102"        , LedDeviceAPA102);
	REGISTER("lpd6803"       , LedDeviceLpd6803);
	REGISTER("lpd8806"       , LedDeviceLpd8806);
	REGISTER("p9813"         , LedDeviceP9813);
	REGISTER("ws2801"        , LedDeviceWs2801);
	REGISTER("ws2812spi"     , LedDeviceWs2812SPI);
	REGISTER("sk6812rgbw-spi", LedDeviceSk6812SPI);
	#endif
	
	// pwm devices
	#ifdef ENABLE_WS2812BPWM
	REGISTER("ws2812b", LedDeviceWS2812b);
	#endif
	#ifdef ENABLE_WS281XPWM
	REGISTER("ws281x" , LedDeviceWS281x);
	#endif

	// network lights
	REGISTER("fadecandy"  , LedDeviceFadeCandy);
	REGISTER("tpm2net"    , LedDeviceTpm2net);
	REGISTER("udpraw"     , LedDeviceUdpRaw);
	REGISTER("e131"       , LedDeviceUdpE131);
	REGISTER("h801"       , LedDeviceUdpH801);
	REGISTER("philipshue" , LedDevicePhilipsHue);
	REGISTER("atmoorb"    , LedDeviceAtmoOrb);
	#ifdef ENABLE_TINKERFORGE
	REGISTER("tinkerforge", LedDeviceTinkerforge);
	#endif

	// direct usb
	REGISTER("hyperion-usbasp", LedDeviceHyperionUsbasp);
	REGISTER("rawhid"         , LedDeviceRawHID);
	REGISTER("paintpack"      , LedDevicePaintpack);
	REGISTER("lightpack"      , LedDeviceLightpack);
	REGISTER("multi-lightpack", LedDeviceMultiLightpack);
	
	// other
	REGISTER("file"     , LedDeviceFile);
	REGISTER("piblaster", LedDevicePiBlaster);
	
	#undef REGISTER

	const LedDeviceRegistry& devList = LedDevice::getDeviceMap();
	LedDevice* device = nullptr;
	try
	{
		for ( auto dev: devList)
		{
			if (dev.first == type)
			{
				device = dev.second(deviceConfig);
				LedDevice::setActiveDevice(dev.first);
				Info(log,"LedDevice '%s' configured.", dev.first.c_str());
				break;
			}
		}
	
		if (device == nullptr)
		{
			Error(log, "Dummy device used, because configured device '%s' is unknown", type.c_str() );
			throw std::runtime_error("unknown device");
		}
	}
	catch(std::exception& e)
	{
		
		Error(log, "Dummy device used, because configured device '%s' throws error '%s'", type.c_str(), e.what());
		const Json::Value dummyDeviceConfig;
		device = LedDeviceFile::construct(Json::nullValue);
	}

	device->open();
	
	return device;
}
