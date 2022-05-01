// hyperion local includes
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
	bool isInitOK {false};

	// Initialise sub-class
	if ( ProviderUdp::init(deviceConfig) )
	{
		_hostName = _devConfig[ CONFIG_HOST ].toString();
		_port = deviceConfig[CONFIG_PORT].toInt(ARTNET_DEFAULT_PORT);

		_artnet_universe = deviceConfig["universe"].toInt(1);
		_artnet_channelsPerFixture = deviceConfig["channelsPerFixture"].toInt(3);

		isInitOK = true;
	}
	return isInitOK;
}

int LedDeviceUdpArtNet::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	{
		if (ProviderUdp::open() == 0)
		{
			// Everything is OK, device is ready
			_isDeviceReady = true;
			retval = 0;
		}
	}
	return retval;
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
	artnet_packet.Sequence	= this_sequence;
	artnet_packet.Physical	= 0;
	artnet_packet.SubUni	= this_universe & 0xff ;
	artnet_packet.Net	= (this_universe >> 8) & 0x7f;
	artnet_packet.Length	= htons(this_dmxChannelCount);
}

int LedDeviceUdpArtNet::write(const std::vector<ColorRgb> &ledValues)
{
	int retVal            = 0;
	int thisUniverse	= _artnet_universe;
	const uint8_t * rawdata = reinterpret_cast<const uint8_t *>(ledValues.data());

/*
This field is incremented in the range 0x01 to 0xff to allow the receiving node to resequence packets.
The Sequence field is set to 0x00 to disable this feature.
*/
	if (_artnet_seq++ == 0)
	{
		_artnet_seq = 1;
	}

	int dmxIdx = 0;			// offset into the current dmx packet

	memset(artnet_packet.raw, 0, sizeof(artnet_packet.raw));
	for (unsigned int ledIdx = 0; ledIdx < _ledRGBCount; ledIdx++)
	{

		artnet_packet.Data[dmxIdx++] = rawdata[ledIdx];
		if ( (ledIdx % 3 == 2) && (ledIdx > 0) )
		{
			dmxIdx += (_artnet_channelsPerFixture-3);
		}

//     is this the   last byte of last packet   ||   last byte of other packets
		if ( (ledIdx == _ledRGBCount-1) || (dmxIdx >= DMX_MAX) )
		{
			prepare(thisUniverse, _artnet_seq, dmxIdx);
			retVal &= writeBytes(18 + qMin(dmxIdx, DMX_MAX), artnet_packet.raw);

			memset(artnet_packet.raw, 0, sizeof(artnet_packet.raw));
			thisUniverse ++;
			dmxIdx = 0;
		}

	}

	return retVal;
}
