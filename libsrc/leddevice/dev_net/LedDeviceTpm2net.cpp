#include "LedDeviceTpm2net.h"
#include <utils/NetUtils.h>

// Constants
namespace {

const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";
const ushort TPM2_DEFAULT_PORT = 65506;
}

LedDeviceTpm2net::LedDeviceTpm2net(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
	, _tpm2_buffer(nullptr)
{
}

LedDeviceTpm2net::~LedDeviceTpm2net()
{
	free (_tpm2_buffer);
}

LedDevice* LedDeviceTpm2net::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceTpm2net(deviceConfig);
}

bool LedDeviceTpm2net::init(const QJsonObject &deviceConfig)
{
	bool isInitOK {false};

	// Initialise sub-class
	if ( ProviderUdp::init(deviceConfig) )
	{
		_hostName = _devConfig[ CONFIG_HOST ].toString();
		_port = deviceConfig[CONFIG_PORT].toInt(TPM2_DEFAULT_PORT);

		_tpm2_max  = deviceConfig["max-packet"].toInt(170);
		_tpm2ByteCount = 3 * _ledCount;
		_tpm2TotalPackets = (_tpm2ByteCount / _tpm2_max) + ((_tpm2ByteCount % _tpm2_max) != 0);

		_tpm2_buffer = (uint8_t*) malloc(_tpm2_max+7);

		isInitOK = true;
	}
	return isInitOK;
}

int LedDeviceTpm2net::open()
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

int LedDeviceTpm2net::write(const std::vector<ColorRgb> &ledValues)
{
	int retVal = 0;

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
