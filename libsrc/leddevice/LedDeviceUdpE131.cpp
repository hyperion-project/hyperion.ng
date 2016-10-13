#include <arpa/inet.h>
#include <QHostInfo>

// hyperion local includes
#include "LedDeviceUdpE131.h"

LedDeviceUdpE131::LedDeviceUdpE131(const QJsonObject &deviceConfig)
	: ProviderUdp()
{
	_deviceReady = init(deviceConfig);
}

bool LedDeviceUdpE131::init(const QJsonObject &deviceConfig)
{
	_LatchTime_ns = 104000;
	_port = 5568;
	ProviderUdp::init(deviceConfig);
	_e131_universe = deviceConfig["universe"].toInt(1);
	_e131_source_name = deviceConfig["source-name"].toString("hyperion on "+QHostInfo::localHostName()).toStdString();
	QString _json_cid = deviceConfig["cid"].toString("");

	if (_json_cid.isEmpty()) 
	{
		_e131_cid = QUuid::createUuid();
		Debug( _log, "e131 no cid found, generated %s", _e131_cid.toString().toStdString().c_str());
	} else {
		_e131_cid = QUuid(_json_cid);
		Debug( _log, "e131  cid found, using %s", _e131_cid.toString().toStdString().c_str());
	}

	return true;
}

LedDevice* LedDeviceUdpE131::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpE131(deviceConfig);
}


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
	memcpy (e131_packet.cid, _e131_cid.toRfc4122().constData() , sizeof(e131_packet.cid) );

	/* Frame Layer */
	e131_packet.frame_flength = htons(0x7000 | (88+this_dmxChannelCount));
	e131_packet.frame_vector = htonl(VECTOR_E131_DATA_PACKET);
	snprintf (e131_packet.source_name, sizeof(e131_packet.source_name), "%s", _e131_source_name.c_str() );
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
	int retVal            = 0;
	int thisChannelCount = 0;
	int dmxChannelCount  = _ledRGBCount;
	const uint8_t * rawdata = reinterpret_cast<const uint8_t *>(ledValues.data());

	_e131_seq++;

	for (int rawIdx = 0; rawIdx < dmxChannelCount; rawIdx++)
	{
		if (rawIdx % DMX_MAX == 0) // start of new packet
		{
			thisChannelCount = (dmxChannelCount - rawIdx < DMX_MAX) ? dmxChannelCount % DMX_MAX : DMX_MAX;
//			                       is this the last packet?         ?       ^^ last packet      : ^^ earlier packets

			prepare(_e131_universe + rawIdx / DMX_MAX, thisChannelCount);
			e131_packet.sequence_number = _e131_seq;
		}

		e131_packet.property_values[1 + rawIdx%DMX_MAX] = rawdata[rawIdx];

//     is this the      last byte of last packet    ||   last byte of other packets
		if ( (rawIdx == dmxChannelCount-1) || (rawIdx %DMX_MAX == DMX_MAX-1) )
		{
#undef e131debug
#if e131debug
			Debug (_log, "send packet: rawidx %d dmxchannelcount %d universe: %d, packetsz %d"
				, rawIdx
				, dmxChannelCount
				, _e131_universe + rawIdx / DMX_MAX
				, E131_DMP_DATA + 1 + thisChannelCount
				);
#endif
			retVal &= writeBytes(E131_DMP_DATA + 1 + thisChannelCount, e131_packet.raw);
		}
	}

	return retVal;
}

