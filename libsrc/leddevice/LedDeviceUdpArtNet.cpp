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


	return true;
}

LedDevice* LedDeviceUdpArtNet::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpArtNet(deviceConfig);
}


// populates the headers
void LedDeviceUdpArtNet::prepare(const unsigned this_universe, const unsigned this_dmxChannelCount)
{
	memset(artnet_packet.raw, 0, sizeof(artnet_packet.raw));

	memcpy (artnet_packet.ID, "Art-Net\0", 8);

	artnet_packet.OpCode	= 0x5000;	// OpOutput / OpDmx
	artnet_packet.ProtVer	= 0x0e00;
	artnet_packet.Sequence	= 0;
	artnet_packet.Physical	= 0;
	artnet_packet.SubUni	= this_universe & 0xff ;
	artnet_packet.Net	= (this_universe >> 8) & 0x7f;
	artnet_packet.Length	= htons(this_dmxChannelCount);

}

int LedDeviceUdpArtNet::write(const std::vector<ColorRgb> &ledValues)
{
	int retVal            = 0;
	int thisChannelCount = 0;
	int dmxChannelCount  = _ledRGBCount;
	const uint8_t * rawdata = reinterpret_cast<const uint8_t *>(ledValues.data());

/*
This field is incremented in the range 0x01 to 0xff to allow the receiving node to resequence packets.
The Sequence field is set to 0x00 to disable this feature.
*/
	if (_artnet_seq++ == 0)
	{
		_artnet_seq = 1;
	}

	for (int rawIdx = 0; rawIdx < dmxChannelCount; rawIdx++)
	{
		if (rawIdx % DMX_MAX == 0) // start of new packet
		{
			thisChannelCount = (dmxChannelCount - rawIdx < DMX_MAX) ? dmxChannelCount % DMX_MAX : DMX_MAX;
//			                       is this the last packet?         ?       ^^ last packet      : ^^ earlier packets

			prepare(_artnet_universe + rawIdx / DMX_MAX, thisChannelCount);
			artnet_packet.Sequence = _artnet_seq;
		}

		artnet_packet.Data[rawIdx%DMX_MAX] = rawdata[rawIdx];

//     is this the     last byte of last packet    ||   last byte of other packets
		if ( (rawIdx == dmxChannelCount-1) || (rawIdx %DMX_MAX == DMX_MAX-1) )
		{
			retVal &= writeBytes(18 + thisChannelCount, artnet_packet.raw);
		}
	}

	return retVal;
}

