#pragma once

#include <QObject>

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
	/// Constructs specific LedDevice
	///
	ProviderHID();

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~ProviderHID();

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig);

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open();
protected:
	/**
	 * Writes the given bytes to the HID-device and
	 *
	 * @param[in] size The length of the data
	 * @param[in] data The data
	 *
	 * @return Zero on succes else negative
	 */
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
};
