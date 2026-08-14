#ifndef PROVIDERHID_H
#define PROVIDERHID_H

// libusb include
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
	///
	explicit ProviderHID(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~ProviderHID() override;

	///
	/// @brief Discover HIB (USB) devices available (for configuration).
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
	/// @brief Write the given bytes to the HID-device
	///
	/// @param[in[ size The length of the data
	/// @param[in] data The data
	/// @return Zero on success, else negative
	///
	int writeBytes(unsigned size, const uint8_t *data);

	/// @brief Enumerate HID interfaces, optionally filtered by VID/PID.
	QJsonArray enumerateHidDevices(
		unsigned short vendorId = 0, unsigned short productId = 0) const;

	/// @brief Enumerate HID interfaces filtered by VID/PID and usage.
	QJsonArray enumerateHidDevices(
		unsigned short vendorId, unsigned short productId,
		unsigned short usagePage, unsigned short usage) const;

	/// @brief Convert an optional HID wide string to a QString.
	static QString fromWide(const wchar_t* value);

	// HID VID and PID
	unsigned short _VendorId;
	unsigned short _ProductId;
	bool           _useFeature;

	/// libusb device handle
	hid_device * _deviceHandle;

	/// Sleep after the connect before continuing
	int _delayAfterConnect_ms;

	bool _blockedForDelay;

private:
	QJsonArray enumerateHidDevices(
		unsigned short vendorId, unsigned short productId,
		unsigned short usagePage, unsigned short usage, bool filterByUsage) const;

private slots:
	/// Unblock the device after a connection delay
	void unblockAfterDelay();

};

#endif // PROVIDERHID_H
