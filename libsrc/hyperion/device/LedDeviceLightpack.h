#pragma once

// stl includes
#include <vector>
#include <cstdint>
#include <string>

// libusb include
#include <libusb.h>

// Hyperion includes
#include <hyperion/LedDevice.h>

///
/// LedDevice implementation for a lightpack device (http://code.google.com/p/light-pack/)
///
class LedDeviceLightpack : public LedDevice
{
public:
	///
	/// Constructs the LedDeviceLightpack
	///
	LedDeviceLightpack(const std::string & serialNumber = "");

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedDeviceLightpack();

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
	struct Version
	{
		int majorVersion;
		int minorVersion;
	};

	/// write bytes to the device
	int writeBytes(uint8_t *data, int size);

	/// Disable the internal smoothing on the Lightpack device
	int disableSmoothing();

	static libusb_device_handle * openDevice(libusb_device * device);
	static std::string getString(libusb_device * device, int stringDescriptorIndex);
	static Version getVersion(libusb_device * device);

private:
	/// libusb context
	libusb_context * _libusbContext;

	/// libusb device handle
	libusb_device_handle * _deviceHandle;

	/// harware bus number
	int _busNumber;

	/// hardware address number
	int  _addressNumber;

	/// device serial number
	std::string _serialNumber;

	/// firmware version of the device
	Version _firmwareVersion;

	/// the number of leds of the device
	int _ledCount;

	/// the number of bits per channel
	int _bitsPerChannel;

	/// buffer for led data
	std::vector<uint8_t> _ledBuffer;
};
