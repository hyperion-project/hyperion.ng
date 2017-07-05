#pragma once

// hyperion includes
#include "ProviderUdp.h"

#include <QUuid>

/**
 *
 *  This program is provided free for you to use in any way that you wish,
 *  subject to the laws and regulations where you are using it.  Due diligence
 *  is strongly suggested before using this code.  Please give credit where due.
 *
 **/

#define ArtNet_DEFAULT_PORT	5568

#define DMX_MAX			512        // 512 usable slots

// http://stackoverflow.com/questions/16396013/artnet-packet-structure
typedef union
{
	struct {
		char		ID[8];		// "Art-Net"
		uint16_t	OpCode;		// See Doc. Table 1 - OpCodes eg. 0x5000 OpOutput / OpDmx
		uint16_t	ProtVer;	// 0x0e00 (aka 14)
		uint8_t		Sequence;	// monotonic counter
		uint8_t		Physical;	// 0x00
		uint8_t		SubUni;		// low universe (0-255)
		uint8_t		Net;		// high universe (not used)
		uint16_t	Length;		// data length (2 - 512)
		uint8_t		Data[ DMX_MAX ];	// universe data
	} __attribute__((packed));

	uint8_t raw[ 18 + DMX_MAX ];

} artnet_packet_t;

///
/// Implementation of the LedDevice interface for sending led colors via udp/E1.31 packets
///
class LedDeviceUdpArtNet : public ProviderUdp
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceUdpArtNet(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig);

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

	void prepare(const unsigned this_universe, const unsigned this_sequence, const unsigned this_dmxChannelCount);


	artnet_packet_t artnet_packet;
	uint8_t _artnet_seq = 1;
	uint8_t _artnet_channelsPerFixture = 3;
	unsigned _artnet_universe = 1;
};
