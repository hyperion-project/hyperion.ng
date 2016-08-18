
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

// hyperion local includes
#include "LedDeviceUdpE131.h"

LedDeviceUdpE131::LedDeviceUdpE131(const std::string& outputDevice, const unsigned latchTime)
	: LedUdpDevice(outputDevice, latchTime)
{
	// empty
}

int LedDeviceUdpE131::write(const std::vector<ColorRgb> &ledValues)
{
	_ledCount = ledValues.size();
	memset(e131_packet.raw, 0, sizeof(e131_packet.raw));

        /* Root Layer */
	e131_packet.preamble_size = htons(16);
        e131_packet.postamble_size = 0;
        memcpy (e131_packet.acn_id, "ASC-E1.17", 12);
        e131_packet.root_flength = htons(0x726e);
        e131_packet.root_vector = htonl(4);
        memcpy (e131_packet.cid, "hyperion!", 16);

        /* Frame Layer */
        e131_packet. frame_flength = htons(0x7258);
        e131_packet. frame_vector = htonl(2);
        memcpy (e131_packet.source_name, "hyperion on hostname", 64);
        e131_packet.priority = 100;
//        e131_packet.reserved;
        e131_packet.sequence_number = e131_seq++;
 //       e131_packet.options;
        e131_packet.universe = htons(1);


        e131_packet. dmp_flength =htons(0x720b);
        e131_packet.dmp_vector = 2;
        e131_packet.type = 0xa1;
        e131_packet.first_address = htons(0);
        e131_packet.address_increment = htons(1);
        e131_packet.property_value_count = htons(513);

	int i=0;
	for (int led_idx=0; led_idx <std::min(_ledCount,170); led_idx++) {
		e131_packet.property_values[i++] = ledValues[led_idx].red;
		e131_packet.property_values[i++] = ledValues[led_idx].green;
		e131_packet.property_values[i++] = ledValues[led_idx].blue;
	}

	const unsigned dataLen = sizeof(e131_packet_t);	

	return writeBytes(dataLen, e131_packet.raw);
}

int LedDeviceUdpE131::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}
