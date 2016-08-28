
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

LedDeviceUdpE131::LedDeviceUdpE131(const Json::Value &deviceConfig)
	: ProviderUdp(deviceConfig)

{
	setConfig(deviceConfig);
}

bool LedDeviceUdpE131::setConfig(const Json::Value &deviceConfig)
{
	ProviderUdp::setConfig(deviceConfig);
	_LatchTime_ns  = deviceConfig.get("latchtime",104000).asInt();
	_e131_universe = deviceConfig.get("universe",1).asInt();

	return true;
}

LedDevice* LedDeviceUdpE131::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceUdpE131(deviceConfig);
}

#define CID "hyperion!\0"
#define SOURCE_NAME "hyperion on hostname\0"

// populates the headers
void LedDeviceUdpE131::prepare(const unsigned this_universe, const unsigned this_dmxChannelCount)
{
	memset(e131_packet.raw, 0, sizeof(e131_packet.raw));

	/* Root Layer */
	e131_packet.preamble_size = htons(16);
	e131_packet.postamble_size = 0;
	memcpy (e131_packet.acn_id, _acn_id, 12);
	e131_packet.root_flength = htons(0x7000 | (110+this_dmxChannelCount) );
	e131_packet.root_vector = htonl(VECTOR_ROOT_E131_DATA);
	memcpy (e131_packet.cid, CID, sizeof(CID) );

	/* Frame Layer */
	e131_packet.frame_flength = htons(0x7000 | (88+this_dmxChannelCount));
	e131_packet.frame_vector = htonl(VECTOR_E131_DATA_PACKET);
	memcpy (e131_packet.source_name, SOURCE_NAME, sizeof(SOURCE_NAME));
	e131_packet.priority = 100;
	e131_packet.reserved = htons(0);
	e131_packet.options = 0;	// Bit 7 =  Preview_Data
					// Bit 6 =  Stream_Terminated
					// Bit 5 = Force_Synchronization
	e131_packet.universe = htons(this_universe);

	/* DMX Layer */
	e131_packet.dmp_flength = htons(0x7000 | (11+this_dmxChannelCount));
	e131_packet.dmp_vector = VECTOR_DMP_SET_PROPERTY;
	e131_packet.type = 0xa1;
	e131_packet.first_address = htons(0);
	e131_packet.address_increment = htons(1);
	e131_packet.property_value_count = htons(1+this_dmxChannelCount);

	e131_packet.property_values[0] = 0;	// start code
}

int LedDeviceUdpE131::write(const std::vector<ColorRgb> &ledValues)
{
	int retVal = 0;

	int _thisChannelCount = 0;

	_e131_seq++;

	const uint8_t * rawdata = reinterpret_cast<const uint8_t *>(ledValues.data());

	_ledCount = ledValues.size();

	int _dmxChannelCount = 3 * _ledCount;

	for (int rawIdx = 0; rawIdx < _dmxChannelCount; rawIdx++)
	{
		if (rawIdx % DMX_MAX == 0) // start of new packet
		{
			_thisChannelCount = (_dmxChannelCount - rawIdx < DMX_MAX) ? _dmxChannelCount % DMX_MAX : DMX_MAX;
//			                     is this the last packet?         ?    ^^ last packet      : ^^ earlier packets

			prepare(_e131_universe + rawIdx / DMX_MAX, _thisChannelCount);
			e131_packet.sequence_number = _e131_seq;
		}

		e131_packet.property_values[1 + rawIdx%DMX_MAX] = rawdata[rawIdx];

//     is this the      last byte of last packet    ||   last byte of other packets
		if ( (rawIdx == _dmxChannelCount-1) || (rawIdx %DMX_MAX == DMX_MAX-1) )
		{
#undef e131debug
#if e131debug
			printf ( "send packet: rawidx %d dmxchannelcount %d universe: %d, packetsz %d\n"
				, rawIdx
				, _dmxChannelCount
				, _e131_universe + rawIdx / DMX_MAX
				, E131_DMP_DATA + 1 + _thisChannelCount
				);
#endif
			retVal &= writeBytes(E131_DMP_DATA + 1 + _thisChannelCount, e131_packet.raw);
		}
	}

	return retVal;
}

int LedDeviceUdpE131::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}
