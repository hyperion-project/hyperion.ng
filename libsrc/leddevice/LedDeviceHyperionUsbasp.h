#pragma once

// stl includes
#include <vector>
#include <cstdint>
#include <string>

// libusb include
#include <libusb.h>

// Hyperion includes
#include <leddevice/LedDevice.h>

///
/// LedDevice implementation for a lightpack device (http://code.google.com/p/light-pack/)
///
class LedDeviceHyperionUsbasp : public LedDevice
{
public:
	// Commands to the Device
	enum Commands {
		CMD_WRITE_WS2801 = 10,
		CMD_WRITE_WS2812 = 11
	};

	///
	/// Constructs the LedDeviceLightpack
	///
	LedDeviceHyperionUsbasp(uint8_t writeLedsCommand);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedDeviceHyperionUsbasp();

	///
	/// Opens and configures the output device
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
	///
	/// Test if the device is a Hyperion Usbasp device
	///
	/// @return Zero on succes else negative
	///
	int testAndOpen(libusb_device * device);

	static libusb_device_handle * openDevice(libusb_device * device);

	static std::string getString(libusb_device * device, int stringDescriptorIndex);

private:
	/// command to write the leds
	const uint8_t _writeLedsCommand;

	/// libusb context
	libusb_context * _libusbContext;

	/// libusb device handle
	libusb_device_handle * _deviceHandle;

	/// Number of leds
	int _ledCount;

	/// Usb device identifiers
	static uint16_t _usbVendorId;
	static uint16_t _usbProductId;
	static std::string _usbProductDescription;
};
