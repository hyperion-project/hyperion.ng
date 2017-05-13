#include <arpa/inet.h>
#include <QHostInfo>

// hyperion local includes
#include "LedDeviceUdpArtNet.h"

LedDeviceUdpArtNet::LedDeviceUdpArtNet(const QJsonObject &deviceConfig)
	: ProviderUdp()
{
	_deviceReady = init(deviceConfig);
}

bool LedDeviceUdpArtNet::init(const QJsonObject &deviceConfig)
{
	_port = 6454;
	ProviderUdp::init(deviceConfig);
	_artnet_universe = deviceConfig["universe"].toInt(1);
//	_artnet_source_name = deviceConfig["source-name"].toString("hyperion on "+QHostInfo::localHostName());


	return true;
}

LedDevice* LedDeviceUdpArtNet::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpArtNet(deviceConfig);
}


// populates the headers
void LedDeviceUdpArtNet::prepare(const unsigned this_universe, const unsigned this_dmxChannelCount)
{
	memset(artnet_packet.data, 0, sizeof(artnet_packet.data));

	memcpy (artnet_packet.ID, _acn_id, 8);
	artnet_packet.OpCode = 0x5000;	// OpOutput / OpDmx
	artnet_packet.version = 0x0e00;
	artnet_packet.seq = 0;
	artnet_packet.physical = 0;
	artnet_packet.subUni = this_universe;
	artnet_packet.net = 0;
	artnet_packet.length = 0;

}

int LedDeviceUdpArtNet::write(const std::vector<ColorRgb> &ledValues)
{
	int retVal            = 0;
	int thisChannelCount = 0;
	int dmxChannelCount  = _ledRGBCount;
	const uint8_t * rawdata = reinterpret_cast<const uint8_t *>(ledValues.data());

	_artnet_seq++;

	for (int rawIdx = 0; rawIdx < dmxChannelCount; rawIdx++)
	{
		if (rawIdx % DMX_MAX == 0) // start of new packet
		{
			thisChannelCount = (dmxChannelCount - rawIdx < DMX_MAX) ? dmxChannelCount % DMX_MAX : DMX_MAX;
//			                       is this the last packet?         ?       ^^ last packet      : ^^ earlier packets

			prepare(_artnet_universe + rawIdx / DMX_MAX, thisChannelCount);
			artnet_packet.seq = _artnet_seq;
		}

		artnet_packet.data[rawIdx%DMX_MAX] = rawdata[rawIdx];

//     is this the      last byte of last packet    ||   last byte of other packets
		if ( (rawIdx == dmxChannelCount-1) || (rawIdx %DMX_MAX == DMX_MAX-1) )
		{
#undef artnetdebug
#if artnetdebug
			Debug (_log, "send packet: rawidx %d dmxchannelcount %d universe: %d, packetsz %d"
				, rawIdx
				, dmxChannelCount
				, _artnet_universe + rawIdx / DMX_MAX
				, ArtNet_DMP_DATA + 1 + thisChannelCount
				);
#endif
			retVal &= writeBytes(19 + thisChannelCount, artnet_packet.data);
		}
	}

	return retVal;
}

