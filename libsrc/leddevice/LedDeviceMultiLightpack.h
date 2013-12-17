#pragma once

// stl includes
#include <vector>
#include <cstdint>
#include <string>
#include <list>

// libusb include
#include <libusb.h>

// Hyperion includes
#include <leddevice/LedDevice.h>
#include "LedDeviceLightpack.h"

///
/// LedDevice implementation for multiple lightpack devices
///
class LedDeviceMultiLightpack : public LedDevice
{
public:
	///
	/// Constructs the LedDeviceMultiLightpack
	///
	LedDeviceMultiLightpack();

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedDeviceMultiLightpack();

	///
	/// Opens and configures the output device7
	///
	/// @return Zero on succes else negative
	///
	int open();

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues);

	///
	/// Switch the leds off
	///
	/// @return Zero on success else negative
	///
	virtual int switchOff();

private:
	static std::list<std::string> getLightpackSerials();
	static std::string getString(libusb_device * device, int stringDescriptorIndex);

private:
	/// buffer for led data
	std::vector<LedDeviceLightpack *> _lightpacks;
};
