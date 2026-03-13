#include "LedDeviceUdpDdp.h"

#include <QtEndian>

#include <utils/NetUtils.h>
#include <utils/RgbToRgbw.h>

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

} //End of constants

namespace DDP {

	// DDP protocol header definitions
	struct Header
	{
		uint8_t flags1;
		uint8_t flags2;
		uint8_t type;
		uint8_t id;
		uint8_t offset[4];
		uint8_t len[2];
	};

	constexpr int HEADER_LEN = (sizeof(struct Header)); // header is 10 bytes (14 if TIME flag used)
	constexpr int MAX_LEDS = 480;
	constexpr int CHANNELS_PER_PACKET = MAX_LEDS * 3;

	namespace flags1
	{
		constexpr auto VER_MASK = 0xc0;
		constexpr auto VER1 = 0x40;
		constexpr auto PUSH = 0x01;
		constexpr auto QUERY = 0x02;
		constexpr auto REPLY = 0x04;
		constexpr auto STORAGE = 0x08;
		constexpr auto TIME = 0x10;
	} // namespace flags1

	namespace id
	{
		constexpr auto DISPLAY = 1;
		constexpr auto CONTROL = 246;
		constexpr auto CONFIG = 250;
		constexpr auto STATUS = 251;
		constexpr auto DMXTRANSIT = 254;
		constexpr auto ALLDEVICES = 255;
	} // namespace id

	enum class LEDDataType : uint8_t
	{
		Undefined = 0,
		RGB = 1,
		HSL = 2,
		RGBW = 3,
		Grayscale = 4
	};

	enum class LEDPixelFormat : uint8_t
	{
		Undefined = 0,
		Pixel1 = 1,
		Pixel4 = 2,
		Pixel8 = 3,
		Pixel16 = 4,
		Pixel24 = 5,
		Pixel32 = 6
	};

// Using "pragma pack" ensures no extra padding is added by the compiler
#pragma pack(push, 1)
		struct PixelDataType
		{
			// Note: Fields defined from LSB to MSB to match "C R TTT SSS"
			// when interpreted as a single byte.
			LEDPixelFormat dataSize : 3; // SSS
			LEDDataType dataType : 3;	 // TTT
			uint8_t reserved : 1;		 // R
			bool customerDefined : 1;	 // C

			// Helper to convert to a standard byte for Qt functions
			uint8_t toRawByte() const
			{
				return *reinterpret_cast<const uint8_t *>(this);
			}
		};
#pragma pack(pop)

} // namespace DDP

LedDeviceUdpDdp::LedDeviceUdpDdp(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
	  ,_packageSequenceNumber(0)
	  ,_whiteAlgorithm(RGBW::WhiteAlgorithm::INVALID)
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

	DDP::PixelDataType p;
	p.customerDefined = true;
	p.reserved = 0;
	if (_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF)
	{
		p.dataType = DDP::LEDDataType::RGB;
	}
	else
	{
		p.dataType = DDP::LEDDataType::RGBW;
	}	
	p.dataSize = DDP::LEDPixelFormat::Pixel8;

	_ddpData.resize(DDP::HEADER_LEN + DDP::CHANNELS_PER_PACKET);
	_ddpData[0] = DDP::flags1::VER1; // flags1
	_ddpData[1] = 0;				 // flags2
	_ddpData[2] = qToBigEndian<uint8_t>(p.toRawByte());	 // type
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

	int channelCount;
	const char* dataPtr = nullptr;

	if (_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF)
	{
		channelCount = _ledRGBCount; // 1 channel for every R,G,B value
		dataPtr =reinterpret_cast<const char*>(ledValues.data());
	}
	else
	{
		channelCount = _ledRGBWCount; // 1 channel for every R,G,B,W value

		QVector<ColorRgbw> rgbwLedValues;
		rgbwLedValues.resize(_ledCount);
		for (int i = 0; i < _ledCount; ++i)
		{
			RGBW::Rgb_to_Rgbw(ledValues[i], &rgbwLedValues[i], _whiteAlgorithm);
		}
		dataPtr = reinterpret_cast<const char*>(rgbwLedValues.data());
	}

	int packetCount = (channelCount + DDP::CHANNELS_PER_PACKET - 1) / DDP::CHANNELS_PER_PACKET;
	int channel = 0;

	_ddpData[0] = DDP::flags1::VER1;

	for (int currentPacket = 0; currentPacket < packetCount; ++currentPacket)
	{
		_packageSequenceNumber &= 0x0F;

		const bool isLastPacket = (currentPacket == packetCount - 1);
		const int packetSize = isLastPacket ? (channelCount - channel) : DDP::CHANNELS_PER_PACKET;

		/*0*/_ddpData[0] = DDP::flags1::VER1 | (isLastPacket ? DDP::flags1::PUSH : 0);
		/*1*/_ddpData[1] = static_cast<char>(_packageSequenceNumber++ & 0x0F);
		/*4*/qToBigEndian<quint32>(static_cast<quint32>(channel), _ddpData.data() + 4);
		/*8*/qToBigEndian<quint16>(static_cast<quint16>(packetSize), _ddpData.data() + 8);

		_ddpData.replace(DDP::HEADER_LEN, packetSize, dataPtr + channel, packetSize);
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

