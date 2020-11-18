#ifndef LEDEVICELIGHTPACK_H
#define LEDEVICELIGHTPACK_H

// stl includes
#include <cstdint>

// libusb include
#include <libusb.h>

// Hyperion includes
#include <leddevice/LedDevice.h>

///
/// LedDevice implementation for a lightpack device (http://code.google.com/p/light-pack/)
///
class LedDeviceLightpack : public LedDevice
{
public:

	///
	/// @brief Constructs a Lightpack LED-device
	///
	/// @param serialNumber serial output device
	///
	explicit LedDeviceLightpack(const QString & serialNumber = "");

	///
	/// @brief Constructs a Lightpack LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceLightpack(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~LedDeviceLightpack() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Power-/turn off the Nanoleaf device.
	///
	/// @return True if success
	///
	bool powerOff() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues  Array of RGB values
	/// @param[in] size       The number of RGB values
	///
	/// @return Zero on success, else negative
	///
	int write(const ColorRgb * ledValues, int size);

	///
	/// @brief Get the serial number of the Lightpack
	///
	/// @return Serial Number
	///	///
	const QString & getSerialNumber() const;

	bool isOpen(){ return _isOpen; }

protected:

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

private:

	///
	/// Search for a LightPack Device (first one found or matching a given serial number)
	///
	/// @param[in] requestedSerialNumber serial number of Lightpack to be search
	/// @return True on Lightpack found
	///
	bool searchDevice(libusb_device * device, const QString & requestedSerialNumber);

	/// write bytes to the device
	int writeBytes(uint8_t *data, int size);

	/// Disable the internal smoothing on the Lightpack device
	int disableSmoothing();

	struct Version
	{
		int majorVersion;
		int minorVersion;
	};


	int openDevice(libusb_device *device, libusb_device_handle ** deviceHandle);
	int closeDevice(libusb_device_handle * deviceHandle);

	QString getProperty(libusb_device * device, int stringDescriptorIndex);

	/// libusb context
	libusb_context * _libusbContext;

	/// libusb device
	libusb_device * _device;

	/// libusb device handle
	libusb_device_handle * _deviceHandle;

	/// harware bus number
	int _busNumber;

	/// hardware address number
	int  _addressNumber;

	/// device serial number
	QString _serialNumber;

	/// firmware version of the device
	Version _firmwareVersion;

	/// the number of bits per channel
	int _bitsPerChannel;

	/// count of real hardware leds
	int _hwLedCount;

	bool _isOpen;
};

#endif // LEDEVICELIGHTPACK_H
