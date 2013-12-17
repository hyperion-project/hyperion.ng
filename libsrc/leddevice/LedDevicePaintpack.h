#pragma once

// STL includes
#include <vector>

// libusb include
#include <hidapi/hidapi.h>

// Hyperion includes
#include <leddevice/LedDevice.h>

///
/// LedDevice implementation for a paintpack device ()
///
class LedDevicePaintpack : public LedDevice
{
public:
	/**
	 * Constructs the paintpack device
	 */
	LedDevicePaintpack();

	/**
	 * Destructs the paintpack device, closes USB connection if open
	 */
	virtual ~LedDevicePaintpack();

	/**
	 * Opens the Paintpack device
	 *
	 * @return Zero on succes else negative
	 */
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
	/// libusb device handle
	hid_device * _deviceHandle;

	/// buffer for led data
	std::vector<uint8_t> _ledBuffer;


};
