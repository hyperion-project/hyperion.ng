// code currently disabled. must be ported to new structure
#if 0

#pragma once

// stl includes
#include <vector>
#include <cstdint>
#include <string>

// libusb include
#include <hidapi/hidapi.h>

// Hyperion includes
#include <hyperion/LedDevice.h>

///
/// LedDevice implementation for a lightpack device (http://code.google.com/p/light-pack/)
///
class LedDeviceLightpackHidapi : public LedDevice
{
public:
	///
	/// Constructs the LedDeviceLightpack
	///
	LedDeviceLightpackHidapi();

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedDeviceLightpackHidapi();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open(const std::string & serialNumber = "");

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  Array of RGB values
	/// @param[in] size       The number of RGB values
	///
	/// @return Zero on success else negative
	///
	int write(const ColorRgb * ledValues, int size);

	///
	/// Switch the leds off
	///
	/// @return Zero on success else negative
	///
	virtual int switchOff();

	/// Get the serial of the Lightpack
	const std::string & getSerialNumber() const;

private:
	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues);

	///
	/// Test if the device is a (or the) lightpack we are looking for
	///
	/// @return Zero on succes else negative
	///
	int testAndOpen(hid_device_info * device, const std::string & requestedSerialNumber);

	/// write bytes to the device
	int writeBytes(uint8_t *data, int size);

	/// Disable the internal smoothing on the Lightpack device
	int disableSmoothing();

	struct Version
	{
		int majorVersion;
		int minorVersion;
	};

	/// libusb device handle
	hid_device * _deviceHandle;

	/// device serial number
	std::string _serialNumber;

	/// firmware version of the device
	Version _firmwareVersion;

	/// the number of leds of the device
	int _hwLedCount;

	/// the number of bits per channel
	int _bitsPerChannel;
};
#endif
