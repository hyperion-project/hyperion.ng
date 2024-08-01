#ifndef LEDEVICET_APA102_H
#define LEDEVICET_APA102_H
#include "ProviderFtdi.h"

class LedDeviceAPA102_ftdi : public ProviderFtdi
{
	Q_OBJECT

public:

	///
	/// @brief Constructs an APA102 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceAPA102_ftdi(const QJsonObject& deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject& deviceConfig);

private:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject& deviceConfig) override;

	void CreateHeader();

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb>& ledValues) override;

	/// The brighness level. Possibile values 1 .. 31.
	int _brightnessControlMaxLevel;

};

#endif // LEDEVICET_APA102_H
