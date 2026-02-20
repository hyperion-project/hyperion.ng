#ifndef LEDEVICEUDPARTNET_H
#define LEDEVICEUDPARTNET_H

// hyperion includes
#include "ProviderUdp.h"

#include <QUuid>
#include <QJsonObject>
#include <QVector>

#include <utils/RgbToRgbw.h>

/**
 *
 *  This program is provided free for you to use in any way that you wish,
 *  subject to the laws and regulations where you are using it.  Due diligence
 *  is strongly suggested before using this code.  Please give credit where due.
 *
 **/

const int DMX_MAX = 512; // 512 usable slots

// http://stackoverflow.com/questions/16396013/artnet-packet-structure
struct artnet_packet_t
{
#pragma pack(push, 1)
	struct {
		char		ID[8];		// "Art-Net"
		uint16_t	OpCode;		// See Doc. Table 1 - OpCodes e.g. 0x5000 OpOutput / OpDmx
		uint16_t	ProtVer;	// 0x0e00 (aka 14)
		uint8_t		Sequence;	// monotonic counter
		uint8_t		Physical;	// 0x00
		uint8_t		SubUni;		// low universe (0-255)
		uint8_t		Net;		// high universe (not used)
		uint16_t	Length;		// data length (2 - 512)
		uint8_t		Data[ DMX_MAX ];	// universe data
	} header;
#pragma pack(pop)

	uint8_t raw[ 18 + DMX_MAX ];
};

///
/// Implementation of the LedDevice interface for sending LED colors to an Art-Net LED-device via UDP
///
class LedDeviceUdpArtNet : public ProviderUdp
{
public:

	///
	/// @brief Constructs an Art-Net LED-device fed via UDP
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceUdpArtNet(const QJsonObject &deviceConfig);

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
	int write(const QVector<ColorRgb> & ledValues) override;

	///
	/// @brief Generate Art-Net communication header
	///
	void prepare(unsigned this_universe, unsigned this_sequence, unsigned this_dmxChannelCount);

	int _ledChannelsPerFixture = 3;

	artnet_packet_t artnet_packet;
	uint8_t _artnet_seq = 1;
	int _artnet_channelsPerFixture = _ledChannelsPerFixture;
	int _artnet_universe = 1;


	// RGBW specific members
	RGBW::WhiteAlgorithm _whiteAlgorithm;
	ColorRgbw _temp_rgbw;
};

#endif // LEDEVICEUDPARTNET_H
