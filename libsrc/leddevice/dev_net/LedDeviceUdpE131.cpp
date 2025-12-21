#include "LedDeviceUdpE131.h"

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

const ushort E131_DEFAULT_PORT = 5568;

/* defined parameters from http://tsp.esta.org/tsp/documents/docs/BSR_E1-31-20xx_CP-2014-1009r2.pdf */
const uint32_t VECTOR_ROOT_E131_DATA = 0x00000004;

const uint8_t VECTOR_DMP_SET_PROPERTY = 0x02;
const uint32_t VECTOR_E131_DATA_PACKET = 0x00000002;

#if 0
#define VECTOR_ROOT_E131_EXTENDED               0x00000008
#define VECTOR_E131_EXTENDED_SYNCHRONIZATION    0x00000001
#define VECTOR_E131_EXTENDED_DISCOVERY          0x00000002
#define VECTOR_UNIVERSE_DISCOVERY_UNIVERSE_LIST 0x00000001
#define E131_E131_UNIVERSE_DISCOVERY_INTERVAL   10         // seconds
#define E131_NETWORK_DATA_LOSS_TIMEOUT          2500       // milli econds
#define E131_DISCOVERY_UNIVERSE                 64214
#endif

const int DMX_MAX = 512; // 512 usable slots
}

LedDeviceUdpE131::LedDeviceUdpE131(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
	, _e131_dmx_max(DMX_MAX)	
	, _whiteAlgorithm(RGBW::WhiteAlgorithm::INVALID)
	, _dmxChannelCount(0)
{
}

LedDevice* LedDeviceUdpE131::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpE131(deviceConfig);
}

bool LedDeviceUdpE131::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if ( !ProviderUdp::init(deviceConfig) )
	{
		return false;
	}

	_hostName = _devConfig[ CONFIG_HOST ].toString();
	_port = deviceConfig[CONFIG_PORT].toInt(E131_DEFAULT_PORT);

	_e131_universe = static_cast<uint8_t>(deviceConfig["universe"].toInt(1));
	_e131_source_name = deviceConfig["source-name"].toString("hyperion on "+QHostInfo::localHostName());
	QString _json_cid = deviceConfig["cid"].toString("");

	// Initialize white algorithm
	QString whiteAlgorithmStr = deviceConfig["whiteAlgorithm"].toString("white_off");
	_whiteAlgorithm = RGBW::stringToWhiteAlgorithm(whiteAlgorithmStr);
	if (_whiteAlgorithm == RGBW::WhiteAlgorithm::INVALID)
	{
		QString errortext = QString("unknown whiteAlgorithm: %1").arg(whiteAlgorithmStr);
		this->setInError(errortext);
		return false;
	}
	Debug(_log, "whiteAlgorithm : %s", QSTRING_CSTR(whiteAlgorithmStr));

	if (_json_cid.isEmpty())
	{
		_e131_cid = QUuid::createUuid();
		Debug( _log, "e131 no CID found, generated %s", QSTRING_CSTR(_e131_cid.toString()));
		return true;
	}
	
	_e131_cid = QUuid(_json_cid);
	if ( _e131_cid.isNull() )
	{
		this->setInError("CID configured is not a valid UUID. Format expected is \"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\"");
		return false;
	}
	Debug( _log, "e131  CID found, using %s", QSTRING_CSTR(_e131_cid.toString()));
	
	return true;
}

int LedDeviceUdpE131::open()
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
void LedDeviceUdpE131::prepare(uint16_t this_universe, uint16_t this_dmxChannelCount)
{
	memset(_e131_packet.raw, 0, sizeof(_e131_packet.raw));

	/* Root Layer */
	_e131_packet.frame.preamble_size = htons(16);
	_e131_packet.frame.postamble_size = 0;
	memcpy (_e131_packet.frame.acn_id, _acn_id, 12);
	_e131_packet.frame.root_flength = htons(0x7000 | (110+this_dmxChannelCount) );
	_e131_packet.frame.root_vector = htonl(VECTOR_ROOT_E131_DATA);
	memcpy (_e131_packet.frame.cid, _e131_cid.toRfc4122().constData() , sizeof(_e131_packet.frame.cid) );
	/* Frame Layer */
	_e131_packet.frame.frame_flength = htons(0x7000 | (88+this_dmxChannelCount));
	_e131_packet.frame.frame_vector = htonl(VECTOR_E131_DATA_PACKET);
	snprintf (_e131_packet.frame.source_name, sizeof(_e131_packet.frame.source_name), "%s", QSTRING_CSTR(_e131_source_name) );
	_e131_packet.frame.priority = 100;
	_e131_packet.frame.reserved = htons(0);
	_e131_packet.frame.sequence_number = 0;
	_e131_packet.frame.options = 0;	// Bit 7 =  Preview_Data
					// Bit 6 =  Stream_Terminated
					// Bit 5 = Force_Synchronization
	_e131_packet.frame.universe = htons(this_universe);

	/* DMX Layer */
	_e131_packet.frame.dmp_flength = htons(0x7000 | (11+this_dmxChannelCount));
	_e131_packet.frame.dmp_vector = VECTOR_DMP_SET_PROPERTY;
	_e131_packet.frame.type = 0xa1;
	_e131_packet.frame.first_address = htons(0);
	_e131_packet.frame.address_increment = htons(1);
	_e131_packet.frame.property_value_count = htons(1+this_dmxChannelCount);

	_e131_packet.frame.property_values[0] = 0;	// start code
}

int LedDeviceUdpE131::write(const QVector<ColorRgb> &ledValues)
{
	int retVal = 0;
	uint16_t thisChannelCount = 0;

	uint8_t* rawDataPtr = _ledBuffer.data();

	int currentChannel = 0;
	for (const ColorRgb& color : ledValues)
	{
		if (_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF)
		{
			rawDataPtr[currentChannel++] = color.red;
			rawDataPtr[currentChannel++] = color.green;
			rawDataPtr[currentChannel++] = color.blue;
		}
		else
		{
			RGBW::Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
			rawDataPtr[currentChannel++] = _temp_rgbw.red;
			rawDataPtr[currentChannel++] = _temp_rgbw.green;
			rawDataPtr[currentChannel++] = _temp_rgbw.blue;
			rawDataPtr[currentChannel++] = _temp_rgbw.white;
		}
	}

	_e131_seq++;

	for (uint16_t rawIdx = 0; rawIdx < _dmxChannelCount; rawIdx++)
	{
		if (rawIdx % _e131_dmx_max == 0) // start of new packet
		{
			thisChannelCount = static_cast<uint16_t>((_dmxChannelCount - rawIdx < _e131_dmx_max) ? _dmxChannelCount - rawIdx : _e131_dmx_max);
			// is this the last packet? ? ^^ last packet : ^^ earlier packets

			prepare(_e131_universe + rawIdx / _e131_dmx_max, thisChannelCount);
			_e131_packet.frame.sequence_number = _e131_seq;
		}

		_e131_packet.frame.property_values[1 + rawIdx % _e131_dmx_max] = rawDataPtr[rawIdx];

		// is this the last byte of last packet || last byte of other packets
		if ((rawIdx == _dmxChannelCount - 1) || (rawIdx % _e131_dmx_max == _e131_dmx_max - 1))
		{
			qCDebug(leddevice_write) << QString("send packet: rawidx %1 dmxchannelcount %2 universe: %3, packetsz %4")
											.arg(rawIdx)
											.arg(_dmxChannelCount)
											.arg(_e131_universe + rawIdx / _e131_dmx_max)
											.arg(thisChannelCount);
			retVal &= writeBytes(E131_DMP_DATA + 1 + thisChannelCount, _e131_packet.raw);
		}
	}

	return retVal;
}
