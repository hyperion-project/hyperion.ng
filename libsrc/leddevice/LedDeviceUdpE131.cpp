
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

LedDeviceUdpE131::LedDeviceUdpE131(const std::string& outputDevice, const unsigned latchTime, const unsigned universe)
	: LedUdpDevice(outputDevice, latchTime)
	, _e131_universe(universe)

{
	// empty
}

#define CID "hyperion!\0"
#define SOURCE_NAME "hyperion on hostname\0"

int LedDeviceUdpE131::write(const std::vector<ColorRgb> &ledValues)
{
	_ledCount = ledValues.size();
	int _dmxChannelCount = 3 * std::min(_ledCount,170);	// DMX512 has max 512 channels == 170 RGB leds

	memset(e131_packet.raw, 0, sizeof(e131_packet.raw));

	/* Root Layer */
	e131_packet.preamble_size = htons(16);
	e131_packet.postamble_size = 0;
	memcpy (e131_packet.acn_id, _acn_id, 12);
	e131_packet.root_flength = htons(0x7000 | (110+_dmxChannelCount) );
	e131_packet.root_vector = htonl(VECTOR_ROOT_E131_DATA);
	memcpy (e131_packet.cid, CID, sizeof(CID) );

	/* Frame Layer */
	e131_packet.frame_flength = htons(0x7000 | (88+_dmxChannelCount));
	e131_packet.frame_vector = htonl(VECTOR_E131_DATA_PACKET);
	memcpy (e131_packet.source_name, SOURCE_NAME, sizeof(SOURCE_NAME));
	e131_packet.priority = 100;
	e131_packet.reserved = htons(0);
	e131_packet.sequence_number = _e131_seq++;
	e131_packet.options = 0;	// Bit 7 =  Preview_Data
					// Bit 6 =  Stream_Terminated
					// Bit 5 = Force_Synchronization
	e131_packet.universe = htons(_e131_universe);


	/* DMX Layer */
	e131_packet.dmp_flength = htons(0x7000 | (11+_dmxChannelCount));
	e131_packet.dmp_vector = VECTOR_DMP_SET_PROPERTY;
	e131_packet.type = 0xa1;
	e131_packet.first_address = htons(0);
	e131_packet.address_increment = htons(1);
	e131_packet.property_value_count = htons(1+_dmxChannelCount);

	int led_idx=0;
	e131_packet.property_values[0] = 0;	// start code
	for (int _dmxIdx=1; _dmxIdx <=  _dmxChannelCount; )
	{
		e131_packet.property_values[_dmxIdx++] = ledValues[led_idx].red;
		e131_packet.property_values[_dmxIdx++] = ledValues[led_idx].green;
		e131_packet.property_values[_dmxIdx++] = ledValues[led_idx].blue;
		led_idx ++;
	}
	return writeBytes(E131_DMP_DATA + 1 + _dmxChannelCount, e131_packet.raw);
}

int LedDeviceUdpE131::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}
