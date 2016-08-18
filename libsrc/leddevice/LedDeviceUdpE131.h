#pragma once

// STL includes
#include <string>

// hyperion includes
#include "LedUdpDevice.h"

/*
*
* https://raw.githubusercontent.com/forkineye/ESPixelStick/master/_E131.h
* Project: E131 - E.131 (sACN) library for Arduino
* Copyright (c) 2015 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*/

#define E131_DEFAULT_PORT 5568

/* E1.31 Packet Offsets */
#define E131_ROOT_PREAMBLE_SIZE 0
#define E131_ROOT_POSTAMBLE_SIZE 2
#define E131_ROOT_ID 4
#define E131_ROOT_FLENGTH 16
#define E131_ROOT_VECTOR 18
#define E131_ROOT_CID 22

#define E131_FRAME_FLENGTH 38
#define E131_FRAME_VECTOR 40
#define E131_FRAME_SOURCE 44
#define E131_FRAME_PRIORITY 108
#define E131_FRAME_RESERVED 109
#define E131_FRAME_SEQ 111
#define E131_FRAME_OPT 112
#define E131_FRAME_UNIVERSE 113

#define E131_DMP_FLENGTH 115
#define E131_DMP_VECTOR 117
#define E131_DMP_TYPE 118
#define E131_DMP_ADDR_FIRST 119
#define E131_DMP_ADDR_INC 121
#define E131_DMP_COUNT 123
#define E131_DMP_DATA 125

/* E1.31 Packet Structure */
typedef union {
    struct {
        /* Root Layer */
        uint16_t preamble_size;
        uint16_t postamble_size;
        uint8_t  acn_id[12];
        uint16_t root_flength;
        uint32_t root_vector;
        uint8_t  cid[16];

        /* Frame Layer */
        uint16_t frame_flength;
        uint32_t frame_vector;
        uint8_t  source_name[64];
        uint8_t  priority;
        uint16_t reserved;
        uint8_t  sequence_number;
        uint8_t  options;
        uint16_t universe;

        /* DMP Layer */
        uint16_t dmp_flength;
        uint8_t  dmp_vector;
        uint8_t  type;
        uint16_t first_address;
        uint16_t address_increment;
        uint16_t property_value_count;
        uint8_t  property_values[513];
    } __attribute__((packed));

    uint8_t raw[638];
} e131_packet_t;



///
/// Implementation of the LedDevice interface for sending led colors via udp/E1.31 packets
///
class LedDeviceUdpE131 : public LedUdpDevice
{
public:
	///
	/// Constructs the LedDevice for sending led colors via udp
	///
	/// @param outputDevice hostname:port
	/// @param latchTime 
	///

	LedDeviceUdpE131(const std::string& outputDevice, const unsigned latchTime);

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	e131_packet_t e131_packet;
	uint8_t	e131_seq = 0;
//        uint8_t acn_id[] = "ASC-E1.17";
 //       uint8_t cid[] = "hyperion!";
  //      uint8_t source_name[] = "hyperion on hostname";


};
