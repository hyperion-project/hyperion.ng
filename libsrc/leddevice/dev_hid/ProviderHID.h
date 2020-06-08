#ifndef PROVIDERHID_H
#define PROVIDERHID_H

// libusb include
#include <hidapi/hidapi.h>

// Leddevice includes
#include <leddevice/LedDevice.h>

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
	ProviderHID();

	///
	/// @brief Destructor of the LedDevice
	///
	virtual ~ProviderHID() override;

	///
	/// @brief Discover HIB (USB) devices available (for configuration).
	///
	/// @return A JSON structure holding a list of devices found
	///
	virtual QJsonObject discover() override;

protected:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	virtual bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	virtual int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	virtual int close() override;

	///
	/// @brief Write the given bytes to the HID-device
	///
	/// @param[in[ size The length of the data
	/// @param[in] data The data
	/// @return Zero on success, else negative
	///
	int writeBytes(const unsigned size, const uint8_t *data);

	// HID VID and PID
	unsigned short _VendorId;
	unsigned short _ProductId;
	bool           _useFeature;

	/// libusb device handle
	hid_device * _deviceHandle;

	/// Sleep after the connect before continuing
	int _delayAfterConnect_ms;

	bool _blockedForDelay;

private slots:
	/// Unblock the device after a connection delay
	void unblockAfterDelay();


private:

};

#endif // PROVIDERHID_H
