#ifndef LEDEVICERAWHID_H
#define LEDEVICERAWHID_H

// Qt includes
#include <QTimer>

// hyperion include
#include "ProviderHID.h"

///
/// Implementation of the LedDevice interface for writing to an RawHID led device.
///
class LedDeviceRawHID : public ProviderHID
{
public:

	///
	/// @brief Constructs a Raw-HID LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceRawHID(const QJsonObject &deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

private:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;
};

#endif // LEDEVICERAWHID_H
