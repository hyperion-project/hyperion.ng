#ifndef LEDEVICEHYPERIONUSBASP_H
#define LEDEVICEHYPERIONUSBASP_H

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
	explicit LedDeviceHyperionUsbasp(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig) override;

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	~LedDeviceHyperionUsbasp() override;

public slots:
	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	int close() override;

protected:
	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open() override;

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	int write(const std::vector<ColorRgb>& ledValues) override;

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

	/// libusb device
	libusb_device * _device;

	/// libusb device handle
	libusb_device_handle * _deviceHandle;
};

#endif // LEDEVICEHYPERIONUSBASP_H
