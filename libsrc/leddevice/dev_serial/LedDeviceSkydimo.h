#ifndef LEDEVICESKYDIMO_H
#define LEDEVICESKYDIMO_H

// hyperion includes
#include "ProviderRs232.h"

///
/// Implementation of the LedDevice interface for writing to a Skydimo LED-device.
///
class LedDeviceSkydimo : public ProviderRs232
{
	Q_OBJECT

public:

	///
	/// @brief Constructs a Skydimo LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceSkydimo(const QJsonObject &deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
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
	/// @brief Prepare the protocol's header
	///
	void prepareHeader();

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const QVector<ColorRgb> & ledValues) override;

	qint64 _bufferLength;
};

#endif // LEDEVICESKYDIMO_H
