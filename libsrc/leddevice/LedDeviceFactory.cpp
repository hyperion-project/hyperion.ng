// Stl includes
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
	#include "LedDeviceSk6822SPI.h"
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
#include "LedDeviceDMX.h"
#include "LedDeviceFile.h"
#include "LedDeviceFadeCandy.h"
#include "LedDeviceTpm2net.h"
#include "LedDeviceUdpRaw.h"
#include "LedDeviceUdpE131.h"
#include "LedDeviceUdpArtNet.h"
#include "LedDeviceHyperionUsbasp.h"
#include "LedDevicePhilipsHue.h"
#include "LedDeviceTpm2.h"
#include "LedDeviceAtmo.h"
#include "LedDeviceAtmoOrb.h"
#include "LedDeviceUdpH801.h"

#ifdef ENABLE_WS281XPWM
	#include "LedDeviceWS281x.h"
#endif

LedDevice * LedDeviceFactory::construct(const QJsonObject & deviceConfig, const int ledCount)
{
	Logger * log = Logger::getInstance("LedDevice");
	QJsonDocument config(deviceConfig);
	QString ss(config.toJson(QJsonDocument::Indented));

	QString type = deviceConfig["type"].toString("UNSPECIFIED").toLower();

	// set amount of led to leddevice
	LedDevice::setLedCount(ledCount);

	#define REGISTER(className) LedDevice::addToDeviceMap(QString(#className).toLower(), LedDevice##className::construct);
	// rs232 devices
	REGISTER(Adalight);
	REGISTER(Sedu);
	REGISTER(DMX);
	REGISTER(Tpm2);
	REGISTER(Atmo);

	// spi devices
	#ifdef ENABLE_SPIDEV
	REGISTER(APA102);
	REGISTER(Lpd6803);
	REGISTER(Lpd8806);
	REGISTER(P9813);
	REGISTER(Ws2801);
	REGISTER(Ws2812SPI);
	REGISTER(Sk6812SPI);
	REGISTER(Sk6822SPI);
	#endif
	
	// pwm devices
	#ifdef ENABLE_WS281XPWM
	REGISTER(WS281x);
	#endif

	// network lights
	REGISTER(FadeCandy);
	REGISTER(Tpm2net);
	REGISTER(UdpRaw);
	REGISTER(UdpE131);
	REGISTER(UdpArtNet);
	REGISTER(UdpH801);
	REGISTER(PhilipsHue);
	REGISTER(AtmoOrb);
	#ifdef ENABLE_TINKERFORGE
	REGISTER(Tinkerforge);
	#endif

	// direct usb
	REGISTER(HyperionUsbasp);
	REGISTER(RawHID);
	REGISTER(Paintpack);
	REGISTER(Lightpack);
	REGISTER(MultiLightpack);
	
	// other
	REGISTER(File);
	REGISTER(PiBlaster);
	
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
				Info(log,"LedDevice '%s' configured.", QSTRING_CSTR(dev.first));
				break;
			}
		}
	
		if (device == nullptr)
		{
			Error(log, "Dummy device used, because configured device '%s' is unknown", QSTRING_CSTR(type) );
			throw std::runtime_error("unknown device");
		}
	}
	catch(std::exception& e)
	{
		
		Error(log, "Dummy device used, because configured device '%s' throws error '%s'", QSTRING_CSTR(type), e.what());
		const QJsonObject dummyDeviceConfig;
		device = LedDeviceFile::construct(QJsonObject());
	}

	device->open();
	
	return device;
}
