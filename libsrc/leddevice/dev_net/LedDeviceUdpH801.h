#ifndef LEDEVICEUDPH801_H
#define LEDEVICEUDPH801_H

// hyperion includes
#include "ProviderUdp.h"

///
/// Implementation of the LedDevice interface for sending LED colors to a H801 LED-device via UDP
///
///
class LedDeviceUdpH801: public ProviderUdp
{
public:

	///
	/// @brief Constructs a H801 LED-device fed via UDP
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceUdpH801(const QJsonObject &deviceConfig);

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
	virtual bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

	QList<int> _ids;
	QByteArray _message;
	const int _prefix_size = 2;
	const int _colors = 5;
	const int _id_size = 3;
	const int _suffix_size = 1;

};

#endif // LEDEVICEUDPH801_H
