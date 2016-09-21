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
	#ifdef ENABLE_WS281XPWM
	LedDevice::addToDeviceMap("ws281x", LedDeviceWS281x::construct);
	#endif

	// network lights
	LedDevice::addToDeviceMap("tpm2net", LedDeviceTpm2net::construct);
	LedDevice::addToDeviceMap("udpraw", LedDeviceUdpRaw::construct);
	LedDevice::addToDeviceMap("e131", LedDeviceUdpE131::construct);
	#ifdef ENABLE_TINKERFORGE
	LedDevice::addToDeviceMap("tinkerforge", LedDeviceTinkerforge::construct);
	#endif
	LedDevice::addToDeviceMap("philipshue", LedDevicePhilipsHue::construct);
	LedDevice::addToDeviceMap("atmoorb", LedDeviceAtmoOrb::construct);
	LedDevice::addToDeviceMap("h801", LedDeviceUdpH801::construct);

	// direct usb
	LedDevice::addToDeviceMap("hyperion-usbasp", LedDeviceHyperionUsbasp::construct);
	LedDevice::addToDeviceMap("rawhid", LedDeviceRawHID::construct);
	LedDevice::addToDeviceMap("paintpack", LedDevicePaintpack::construct);
	LedDevice::addToDeviceMap("lightpack", LedDeviceLightpack::construct);
	LedDevice::addToDeviceMap("multi-lightpack", LedDeviceMultiLightpack::construct);
	
	// other
	LedDevice::addToDeviceMap("file", LedDeviceFile::construct);
	LedDevice::addToDeviceMap("piblaster", LedDevicePiBlaster::construct);
	
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
