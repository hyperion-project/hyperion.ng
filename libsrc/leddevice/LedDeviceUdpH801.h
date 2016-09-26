#pragma once

// hyperion includes
#include "ProviderUdp.h"

///
/// Implementation of the LedDevice interface for sending led colors via udp.
///
class LedDeviceUdpH801: public ProviderUdp
{
protected:
	QList<int> _ids;
	QByteArray _message;
	const int _prefix_size = 2;
	const int _colors = 5;
	const int _id_size = 3;
	const int _suffix_size = 1;

public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceUdpH801(const Json::Value &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const Json::Value &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const Json::Value &deviceConfig);

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);
};
