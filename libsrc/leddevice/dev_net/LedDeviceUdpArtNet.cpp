#include "LedDeviceUdpArtNet.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include <QHostInfo>

#include <utils/NetUtils.h>

// Constants
namespace {

const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";

const ushort ARTNET_DEFAULT_PORT = 6454;
}

LedDeviceUdpArtNet::LedDeviceUdpArtNet(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
{
}

LedDevice* LedDeviceUdpArtNet::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpArtNet(deviceConfig);
}

bool LedDeviceUdpArtNet::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if (!ProviderUdp::init(deviceConfig))
	{
		return false;
	}

	_hostName = _devConfig[ CONFIG_HOST ].toString();
	_port = deviceConfig[CONFIG_PORT].toInt(ARTNET_DEFAULT_PORT);

	_artnet_universe = deviceConfig["universe"].toInt(1);
	_artnet_channelsPerFixture = deviceConfig["channelsPerFixture"].toInt(3);

	return true;
}

int LedDeviceUdpArtNet::open()
{
	_isDeviceReady = false;
	this->setIsRecoverable(true);
	
	NetUtils::convertMdnsToIp(_log, _hostName);
	if (ProviderUdp::open() == 0)
	{
		// Everything is OK, device is ready
		_isDeviceReady = true;

	}
	return _isDeviceReady ? 0 : -1;
}

// populates the headers
void LedDeviceUdpArtNet::prepare(unsigned this_universe, unsigned this_sequence, unsigned this_dmxChannelCount)
{
// WTF? why do the specs say:
// "This value should be an even number in the range 2 – 512. "
	if (this_dmxChannelCount & 0x1)
	{
		this_dmxChannelCount++;
	}

	memcpy (artnet_packet.ID, "Art-Net\0", 8);

	artnet_packet.OpCode	= htons(0x0050);	// OpOutput / OpDmx
	artnet_packet.ProtVer	= htons(0x000e);
	artnet_packet.Sequence	= static_cast<uint8_t>(this_sequence);
	artnet_packet.Physical	= 0;
	artnet_packet.SubUni	= this_universe & 0xff ;
	artnet_packet.Net	= (this_universe >> 8) & 0x7f;
	artnet_packet.Length	= htons(static_cast<uint16_t>(this_dmxChannelCount));
}

int LedDeviceUdpArtNet::write(const QVector<ColorRgb> &ledValues)
{
	int retVal = 0;
	int thisUniverse = _artnet_universe;
	auto rawdata = reinterpret_cast<const uint8_t *>(ledValues.data());

	/*
	This field is incremented in the range 0x01 to 0xff to allow the receiving node to resequence packets.
	The Sequence field is set to 0x00 to disable this feature.
	*/
	if (_artnet_seq++ == 0)
	{
		_artnet_seq = 1;
	}

	int dmxIdx = 0; // offset into the current dmx packet

	memset(artnet_packet.raw, 0, sizeof(artnet_packet.raw));
	
	// Iterate over LEDs, not RGB bytes
	for (unsigned int ledIdx = 0; ledIdx < static_cast<unsigned int>(ledValues.size()); ledIdx++)
	{
		// Write R, G, B for this LED
		artnet_packet.Data[dmxIdx++] = rawdata[ledIdx * 3 + 0]; // Red
		artnet_packet.Data[dmxIdx++] = rawdata[ledIdx * 3 + 1]; // Green
		artnet_packet.Data[dmxIdx++] = rawdata[ledIdx * 3 + 2]; // Blue
		
		// Skip extra channels if fixture needs more than RGB
		dmxIdx += (_artnet_channelsPerFixture - 3);

		// is this the last LED of last packet || last LED that fits in current packet
		if ( (ledIdx == static_cast<unsigned int>(ledValues.size()) - 1) || (dmxIdx >= DMX_MAX) )
		{
			prepare(thisUniverse, _artnet_seq, dmxIdx);
			
			if (writeBytes(18 + dmxIdx, artnet_packet.raw) < 0)
			{
				retVal = -1;
			}

			memset(artnet_packet.raw, 0, sizeof(artnet_packet.raw));
			thisUniverse++;
			dmxIdx = 0;
		}
	}

	return retVal;
}
