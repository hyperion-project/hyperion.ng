#pragma once

// hyperion includes
#include "ProviderUdp.h"

#include <QUuid>

/**
 *
 * https://raw.githubusercontent.com/forkineye/ESPixelStick/master/_ArtNet.h
 * Project: ArtNet - E.131 (sACN) library for Arduino
 * Copyright (c) 2015 Shelby Merrick
 * http://www.forkineye.com
 *
 *  This program is provided free for you to use in any way that you wish,
 *  subject to the laws and regulations where you are using it.  Due diligence
 *  is strongly suggested before using this code.  Please give credit where due.
 *
 **/

#define ArtNet_DEFAULT_PORT 5568

/* E1.31 Packet Offsets */
#define ArtNet_ROOT_PREAMBLE_SIZE 0
#define ArtNet_ROOT_POSTAMBLE_SIZE 2
#define ArtNet_ROOT_ID 4
#define ArtNet_ROOT_FLENGTH 16
#define ArtNet_ROOT_VECTOR 18
#define ArtNet_ROOT_CID 22

#define ArtNet_FRAME_FLENGTH 38
#define ArtNet_FRAME_VECTOR 40
#define ArtNet_FRAME_SOURCE 44
#define ArtNet_FRAME_PRIORITY 108
#define ArtNet_FRAME_RESERVED 109
#define ArtNet_FRAME_SEQ 111
#define ArtNet_FRAME_OPT 112
#define ArtNet_FRAME_UNIVERSE 113

#define ArtNet_DMP_FLENGTH 115
#define ArtNet_DMP_VECTOR 117
#define ArtNet_DMP_TYPE 118
#define ArtNet_DMP_ADDR_FIRST 119
#define ArtNet_DMP_ADDR_INC 121
#define ArtNet_DMP_COUNT 123
#define ArtNet_DMP_DATA 125

typedef struct {
	char		ID[8];         //"Art-Net"
	uint16_t	OpCode;      // See Doc. Table 1 - OpCodes eg. 0x5000 OpOutput / OpDmx
	uint16_t	version;     // 0x0e00 (aka 14)
	uint8_t		seq;         // monotonic counter
	uint8_t		physical;    // 0x00
	uint8_t		subUni;      // low universe (0-255)
	uint8_t		net;         // high universe (not used)
	uint16_t	length;      // data length (2 - 512)
	uint8_t		data[512];  // universe data
} artnet_packet_t;

#define DMX_MAX                                 512        // 512 usable slots

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

	void prepare(const unsigned this_universe, const unsigned this_dmxChannelCount);

	artnet_packet_t artnet_packet;
	uint8_t _artnet_seq = 0;
	uint8_t _artnet_universe = 1;
	uint8_t _acn_id[8] = {0x41, 0x72, 0x74, 0x2d, 0x4e, 0x65, 0x74, 0x00 }; //  "Art-Net", 0x00 
	QUuid _artnet_cid;
};
