#include "LedDeviceUdpDdp.h"

#include <QtEndian>

#include <utils/NetUtils.h>

// DDP header format
// header is 10 bytes (14 if TIME flag used)
struct ddp_hdr_struct {
	uint8_t flags1;
	uint8_t flags2;
	uint8_t type;
	uint8_t id;
	uint32_t offset;
	uint16_t len;
};

// Constants
namespace {

const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";

const ushort DDP_DEFAULT_PORT = 4048;

namespace DDP {

	// DDP protocol header definitions
	struct Header {
		uint8_t flags1;
		uint8_t flags2;
		uint8_t type;
		uint8_t id;
		uint8_t offset[4];
		uint8_t len[2];
	};

	constexpr int HEADER_LEN = (sizeof(struct Header)); // header is 10 bytes (14 if TIME flag used)
	constexpr int MAX_LEDS = 480;
	constexpr int CHANNELS_PER_PACKET = MAX_LEDS*3;

	namespace flags1 {
	constexpr auto VER_MASK = 0xc0;
	constexpr auto VER1 = 0x40;
	constexpr auto PUSH = 0x01;
	constexpr auto QUERY = 0x02;
	constexpr auto REPLY = 0x04;
	constexpr auto STORAGE = 0x08;
	constexpr auto TIME = 0x10;
	}  // namespace flags1

	namespace id {
	constexpr auto DISPLAY = 1;
	constexpr auto CONTROL = 246;
	constexpr auto CONFIG = 250;
	constexpr auto STATUS = 251;
	constexpr auto DMXTRANSIT = 254;
	constexpr auto ALLDEVICES = 255;
	}  // namespace id

}  // namespace DDP

} //End of constants

LedDeviceUdpDdp::LedDeviceUdpDdp(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
	  ,_packageSequenceNumber(0)
{
}

LedDevice* LedDeviceUdpDdp::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpDdp(deviceConfig);
}

bool LedDeviceUdpDdp::init(const QJsonObject &deviceConfig)
{
	if (!ProviderUdp::init(deviceConfig))
	{
		return false;
	}

	_hostName = _devConfig[ CONFIG_HOST ].toString();
	_port = deviceConfig[CONFIG_PORT].toInt(DDP_DEFAULT_PORT);

	Debug(_log, "Hostname/IP       : %s", QSTRING_CSTR(_hostName) );
	Debug(_log, "Port              : %d", _port );

	_ddpData.resize(DDP::HEADER_LEN + DDP::CHANNELS_PER_PACKET);
	_ddpData[0] = DDP::flags1::VER1; // flags1
	_ddpData[1] = 0;				 // flags2
	_ddpData[2] = 1;				 // type
	_ddpData[3] = DDP::id::DISPLAY;	 // id

	return true;
}

int LedDeviceUdpDdp::open()
{
	_isDeviceReady = false;
	this->setIsRecoverable(true);	

	if (_hostName.isNull())
	{
		Error(_log, "Empty hostname or IP address. UDP stream cannot be initiatised.");
		return -1;
	}

	NetUtils::convertMdnsToIp(_log, _hostName);
	if (ProviderUdp::open() == 0)
	{
		// Everything is OK, device is ready
		_isDeviceReady = true;
	}

	return _isDeviceReady ? 0 : -1;
}

int LedDeviceUdpDdp::write(const QVector<ColorRgb> &ledValues)
{
	int rc {0};

	int channelCount = static_cast<int>(_ledCount) * 3; // 1 channel for every R,G,B value
	int packetCount = ((channelCount-1) / DDP::CHANNELS_PER_PACKET) + 1;
	int channel = 0;

	_ddpData[0] = DDP::flags1::VER1;

	for (int currentPacket = 0; currentPacket < packetCount; currentPacket++)
	{
		if (_packageSequenceNumber > 15)
		{
			_packageSequenceNumber = 0;
		}

		int packetSize = DDP::CHANNELS_PER_PACKET;

		if (currentPacket == (packetCount - 1))
		{
			// last packet, set the push flag
			/*0*/_ddpData[0] = DDP::flags1::VER1 | DDP::flags1::PUSH;

			if (channelCount % DDP::CHANNELS_PER_PACKET != 0)
			{
				packetSize = channelCount % DDP::CHANNELS_PER_PACKET;
			}
		}

		/*1*/_ddpData[1] = static_cast<char>(_packageSequenceNumber++ & 0x0F);
		/*4*/qToBigEndian<quint32>(static_cast<quint32>(channel), _ddpData.data() + 4);
		/*8*/qToBigEndian<quint16>(static_cast<quint16>(packetSize), _ddpData.data() + 8);

		_ddpData.replace(DDP::HEADER_LEN, channel, reinterpret_cast<const char*>(ledValues.data())+channel, packetSize);
		_ddpData.resize(DDP::HEADER_LEN + packetSize);

		rc = writeBytes(_ddpData);

		if (rc != 0)
		{
			break;
		}
		channel += packetSize;
	}
	return rc;
}

