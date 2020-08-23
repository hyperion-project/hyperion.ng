#ifndef LEDEVICEMULTILIGHTPACK_H
#define LEDEVICEMULTILIGHTPACK_H

// stl includes
#include <vector>
#include <cstdint>
#include <QStringList>
#include <QString>

// libusb include
#include <libusb.h>

// Hyperion includes
#include <leddevice/LedDevice.h>
#include "LedDeviceLightpack.h"

///
/// LedDevice implementation for multiple lightpack devices
///
class LedDeviceMultiLightpack : public LedDevice
{
public:

	///
	/// @brief Constructs a LedDevice of multiple Lightpack LED-devices
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceMultiLightpack(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~LedDeviceMultiLightpack() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

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
	/// @brief Power-/turn off the Nanoleaf device.
	///
	/// @return True if success
	///
	bool powerOff() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

private:

	static QStringList getLightpackSerials();
	static QString getString(libusb_device * device, int stringDescriptorIndex);

	/// buffer for led data
	std::vector<LedDeviceLightpack *> _lightpacks;
};

#endif // LEDEVICEMULTILIGHTPACK_H
