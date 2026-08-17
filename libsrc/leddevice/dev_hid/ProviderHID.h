#ifndef PROVIDERHID_H
#define PROVIDERHID_H

// HIDAPI include
#include <hidapi.h>

// Leddevice includes
#include <leddevice/LedDevice.h>

#include <QJsonArray>

///
/// The ProviderHID implements an abstract base-class for LedDevices using an HID-device.
///
class ProviderHID : public LedDevice
{
	Q_OBJECT

public:

	///
	/// @brief Constructs a HID (USB) LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	/// @param vendorId Fixed HID vendor identifier, or 0 to read it from the configuration
	/// @param productId Fixed HID product identifier, or 0 to read it from the configuration
	///
	explicit ProviderHID(const QJsonObject& deviceConfig, unsigned short vendorId = 0, unsigned short productId = 0);

	///
	/// @brief Destructor of the LedDevice
	///
	~ProviderHID() override;

	///
	/// @brief Discover HID (USB) devices available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject& deviceConfig) override;

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
	/// @brief Write the given bytes to the HID-device
	///
	/// @param[in] size The length of the data
	/// @param[in] data The data
	/// @return Zero on success, else negative
	///
	int writeBytes(unsigned size, const uint8_t* data);

	/// @brief Format a HID identifier as a zero-padded hexadecimal value.
	static QString formatHexValue(unsigned short value);

	/// @brief Convert HID interface information to device properties.
	static QJsonObject hidDeviceInfoToJson(const hid_device_info& deviceInfo);

	// HID VID and PID
	unsigned short _vendorId;
	unsigned short _productId;
	bool           _useFeature;
	QString        _devicePath;

	/// HIDAPI device handle
	hid_device* _deviceHandle;

	/// Sleep after the connect before continuing
	int _delayAfterConnect_ms;

	bool _blockedForDelay;

private:
	static QJsonArray enumerateHidDevices(
		unsigned short vendorId, unsigned short productId,
		unsigned short usagePage, unsigned short usage, bool filterByUsage);

private slots:
	/// Unblock the device after a connection delay
	void unblockAfterDelay();
};

#endif // PROVIDERHID_H
