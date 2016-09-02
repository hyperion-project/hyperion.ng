
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <QHostInfo>
#include <QUuid>

// hyperion local includes
#include "LedDeviceTpm2netnew.h"

LedDeviceTpm2netnew::LedDeviceTpm2netnew(const Json::Value &deviceConfig)
	: ProviderUdp(deviceConfig)

{
	setConfig(deviceConfig);
}

bool LedDeviceTpm2netnew::setConfig(const Json::Value &deviceConfig)
{
	ProviderUdp::setConfig(deviceConfig);
	_LatchTime_ns  = deviceConfig.get("latchtime",104000).asInt();
	_tpm2_max  = deviceConfig.get("max-packet",170).asInt();
	return true;
}

LedDevice* LedDeviceTpm2netnew::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceTpm2netnew(deviceConfig);
}


// populates the headers

int LedDeviceTpm2netnew::write(const std::vector<ColorRgb> &ledValues)
{
	uint8_t * _tpm2_buffer = (uint8_t*) malloc(_tpm2_max+7);

	int retVal = 0;

	_ledCount = ledValues.size();
	_tpm2ByteCount = 3 * _ledCount;
	_tpm2TotalPackets = 1 + _tpm2ByteCount / _tpm2_max;

	int _thisPacketBytes = 0;
	_tpm2ThisPacket = 1;

	const uint8_t * rawdata = reinterpret_cast<const uint8_t *>(ledValues.data());

	for (int rawIdx = 0; rawIdx < _tpm2ByteCount; rawIdx++)
	{
		if (rawIdx % _tpm2_max == 0) // start of new packet
		{
			_thisPacketBytes = (_tpm2ByteCount - rawIdx < _tpm2_max) ? _tpm2ByteCount % _tpm2_max : _tpm2_max;
//			                        is this the last packet?         ?    ^^ last packet          : ^^ earlier packets

			_tpm2_buffer[0] = 0x9c;	// Packet start byte
			_tpm2_buffer[1] = 0xda; // Packet type Data frame
			_tpm2_buffer[2] = (_thisPacketBytes >> 8) & 0xff; // Frame size high
			_tpm2_buffer[3] = _thisPacketBytes & 0xff; // Frame size low
			_tpm2_buffer[4] = _tpm2ThisPacket++; // Packet Number
			_tpm2_buffer[5] = _tpm2TotalPackets; // Number of packets
		}

		_tpm2_buffer [6 + rawIdx%_tpm2_max] = rawdata[rawIdx];

//     is this the      last byte of last packet    ||   last byte of other packets
		if ( (rawIdx == _tpm2ByteCount-1) || (rawIdx %_tpm2_max == _tpm2_max-1) )
		{
			_tpm2_buffer [6 + rawIdx%_tpm2_max +1] = 0x36;		// Packet end byte
			retVal &= writeBytes(_thisPacketBytes+7, _tpm2_buffer);
		}
	}

	return retVal;
}

int LedDeviceTpm2netnew::switchOff()
{
	return write(std::vector<ColorRgb>(_ledCount, ColorRgb{0,0,0}));
}
