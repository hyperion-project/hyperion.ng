#ifndef LEDEVICEUDPDDP_H
#define LEDEVICEUDPDDP_H

// hyperion includes
#include "ProviderUdp.h"

///
/// Implementation of the LedDevice interface for sending LED colors via UDP and the Distributed Display Protocol (DDP)
/// http://www.3waylabs.com/ddp/#Data%20Types
///
class LedDeviceUdpDdp : public virtual ProviderUdp
{
public:

	///
	/// @brief Constructs a LED-device fed via DDP
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceUdpDdp(const QJsonObject &deviceConfig);

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
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

private:

	QByteArray  _ddpData;

	int _packageSequenceNumber;
};

#endif // LEDEVICEUDPDDP_H
