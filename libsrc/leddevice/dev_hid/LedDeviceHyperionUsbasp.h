#pragma once

// stl includes
#include <vector>
#include <cstdint>

// libusb include
#include <libusb.h>

// Hyperion includes
#include <leddevice/LedDevice.h>

///
/// LedDevice implementation for a USBasp programmer with modified firmware (https://github.com/poljvd/hyperion-usbasp)
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
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceHyperionUsbasp(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

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

protected:
	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues);

	///
	/// Test if the device is a Hyperion Usbasp device
	///
	/// @return Zero on succes else negative
	///
	int testAndOpen(libusb_device * device);

	static libusb_device_handle * openDevice(libusb_device * device);

	static QString getString(libusb_device * device, int stringDescriptorIndex);

	/// command to write the leds
	uint8_t _writeLedsCommand;

	/// libusb context
	libusb_context * _libusbContext;

	/// libusb device handle
	libusb_device_handle * _deviceHandle;

	/// Usb device identifiers
	static uint16_t     _usbVendorId;
	static uint16_t     _usbProductId;
	static QString      _usbProductDescription;
};
