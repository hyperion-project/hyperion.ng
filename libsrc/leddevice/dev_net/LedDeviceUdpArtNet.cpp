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

	// Initialize white algorithm
	QString whiteAlgorithmStr = deviceConfig["whiteAlgorithm"].toString("white_off");
	_whiteAlgorithm = RGBW::stringToWhiteAlgorithm(whiteAlgorithmStr);
	if (_whiteAlgorithm == RGBW::WhiteAlgorithm::INVALID)
	{
		QString errortext = QString("Unknown White-Algorithm: %1").arg(whiteAlgorithmStr);
		this->setInError(errortext);
		return false;
	}
	Debug(_log, "White-Algorithm   : %s", QSTRING_CSTR(whiteAlgorithmStr));	
	_ledChannelsPerFixture = (_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF) ? 3 : 4;

	_artnet_universe = deviceConfig["universe"].toInt(1);
	_artnet_channelsPerFixture = deviceConfig["channelsPerFixture"].toInt(_ledChannelsPerFixture);

	// Validate channelsPerFixture
	if (_artnet_channelsPerFixture < _ledChannelsPerFixture)
	{
		Error(_log, "channelsPerFixture must be at least %d. Setting to %d.", _ledChannelsPerFixture, _ledChannelsPerFixture);
		_artnet_channelsPerFixture = _ledChannelsPerFixture;
	}

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

	memcpy (artnet_packet.header.ID, "Art-Net\0", 8);

	artnet_packet.header.OpCode   = htons(0x5000); // OpOutput / OpDmx
	artnet_packet.header.ProtVer  = htons(0x000e);
	artnet_packet.header.Sequence = static_cast<uint8_t>(this_sequence);
	artnet_packet.header.Physical = 0;
	artnet_packet.header.SubUni   = this_universe & 0xff ;
	artnet_packet.header.Net      = (this_universe >> 8) & 0x7f;
	artnet_packet.header.Length   = htons(static_cast<uint16_t>(this_dmxChannelCount));
}

int LedDeviceUdpArtNet::write(const QVector<ColorRgb> &ledValues)
{
	int retVal = 0;
	int thisUniverse = _artnet_universe;

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
	
	for (const ColorRgb& color : ledValues)
	{
		// Check if this LED would overflow the DMX packet
		if (dmxIdx + _artnet_channelsPerFixture > DMX_MAX)
		{
			// Send current packet before continuing
			prepare(thisUniverse, _artnet_seq, dmxIdx);
			
			if (writeBytes(18 + dmxIdx, artnet_packet.raw) < 0)
			{
				retVal = -1;
			}

			memset(artnet_packet.raw, 0, sizeof(artnet_packet.raw));
			thisUniverse++;
			dmxIdx = 0;
		}

		if (_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF)
		{
			artnet_packet.header.Data[dmxIdx++] = color.red;
			artnet_packet.header.Data[dmxIdx++] = color.green;
			artnet_packet.header.Data[dmxIdx++] = color.blue;
		}
		else
		{
			RGBW::Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
			artnet_packet.header.Data[dmxIdx++] = _temp_rgbw.red;
			artnet_packet.header.Data[dmxIdx++] = _temp_rgbw.green;
			artnet_packet.header.Data[dmxIdx++] = _temp_rgbw.blue;
			artnet_packet.header.Data[dmxIdx++] = _temp_rgbw.white;
		}
		
		// Skip extra channels if fixture needs more than RGB/RGBW
		dmxIdx += (_artnet_channelsPerFixture - _ledChannelsPerFixture);
	}

	// Send the last packet if there's any data
	if (dmxIdx > 0)
	{
		prepare(thisUniverse, _artnet_seq, dmxIdx);
		
		if (writeBytes(18 + dmxIdx, artnet_packet.raw) < 0)
		{
			retVal = -1;
		}
	}

	return retVal;
}
