#pragma once

// hyperion includes
#include "ProviderUdp.h"

#define TPM2_DEFAULT_PORT 65506

///
/// Implementation of the LedDevice interface for sending led colors via udp tpm2.net packets
///
class LedDeviceTpm2net : public ProviderUdp
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceTpm2net(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

private:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	int _tpm2_max;
	int _tpm2ByteCount;
	int _tpm2TotalPackets;
	int _tpm2ThisPacket;
};
