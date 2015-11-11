#pragma once

#include <QObject>

// libusb include
#include <hidapi/hidapi.h>

// Leddevice includes
#include <leddevice/LedDevice.h>

///
/// The LedHIDDevice implements an abstract base-class for LedDevices using an HID-device.
///
class LedHIDDevice : public QObject, public LedDevice
{
	Q_OBJECT

public:
	///
	/// Constructs the LedDevice attached to an HID-device
	///
	/// @param[in] VendorId The USB VID of the output device
	/// @param[in] ProductId The USB PID of the output device
	///
	LedHIDDevice(const unsigned short VendorId, const unsigned short ProductId, int delayAfterConnect_ms = 0, const bool useFeature = false);

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~LedHIDDevice();

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
	 * @param[in[ size The length of the data
	 * @param[in] data The data
	 *
	 * @return Zero on succes else negative
	 */
	int writeBytes(const unsigned size, const uint8_t *data);

private slots:
	/// Unblock the device after a connection delay
	void unblockAfterDelay();

private:
	// HID VID and PID
	const unsigned short _VendorId;
	const unsigned short _ProductId;
	const bool _useFeature;

	/// libusb device handle
	hid_device * _deviceHandle;

	/// Sleep after the connect before continuing
	const int _delayAfterConnect_ms;

	bool _blockedForDelay;
};
