#ifndef LEDEVICETADALIGHT_H
#define LEDEVICETADALIGHT_H

// hyperion includes
#include "ProviderRs232.h"

namespace Adalight
{
typedef enum ProtocolType
{
	ADA = 0,
	LBAPA,
	AWA
} PROTOCOLTYPE;
}

///
/// Implementation of the LedDevice interface for writing to an Adalight LED-device.
///
class LedDeviceAdalight : public ProviderRs232
{
	Q_OBJECT

public:

	///
	/// @brief Constructs an Adalight LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceAdalight(const QJsonObject &deviceConfig);

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
	int write(const std::vector<ColorRgb> & ledValues) override;

	void whiteChannelExtension(uint8_t*& writer);

	qint64 _bufferLength;

	Adalight::PROTOCOLTYPE _streamProtocol;

	bool _white_channel_calibration;
	uint8_t _white_channel_limit;
	uint8_t _white_channel_red;
	uint8_t _white_channel_green;
	uint8_t _white_channel_blue;
};

#endif // LEDEVICETADALIGHT_H
