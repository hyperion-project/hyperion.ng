#include "LedDeviceAdalight.h"

#include <QtEndian>

// Constants
namespace {

// Configuration settings
const char CONFIG_STREAM_PROTOCOL[] = "streamProtocol";

const char CONFIG_WHITE_CHANNEL_CALLIBRATION[] = "white_channel_calibration";
const char CONFIG_WHITE_CHANNEL_LIMIT[] = "white_channel_limit";
const char CONFIG_WHITE_CHANNEL_RED[] = "white_channel_red";
const char CONFIG_WHITE_CHANNEL_GREEN[] = "white_channel_green";
const char CONFIG_WHITE_CHANNEL_BLUE[] = "white_channel_blue";
constexpr int HEADER_SIZE {6};

} //End of constants

LedDeviceAdalight::LedDeviceAdalight(const QJsonObject &deviceConfig)
	: ProviderRs232(deviceConfig)
{
}

LedDevice* LedDeviceAdalight::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAdalight(deviceConfig);
}

bool LedDeviceAdalight::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderRs232::init(deviceConfig) )
	{
		_streamProtocol = static_cast<Adalight::PROTOCOLTYPE>(deviceConfig[CONFIG_STREAM_PROTOCOL].toString().toInt());
		switch (_streamProtocol) {

		case Adalight::AWA:
		{
			Debug( _log, "Adalight driver uses Hyperserial protocol");
			_white_channel_calibration  = deviceConfig[CONFIG_WHITE_CHANNEL_CALLIBRATION].toBool(false);
			double _white_channel_limit_percent = deviceConfig[CONFIG_WHITE_CHANNEL_LIMIT].toDouble(1);
			_white_channel_limit  = static_cast<uint8_t>(qMin(qRound(_white_channel_limit_percent * 255.0 / 100.0), 255));
			_white_channel_red  = static_cast<uint8_t>(qMin(deviceConfig[CONFIG_WHITE_CHANNEL_RED].toInt(255), 255));
			_white_channel_green = static_cast<uint8_t>(qMin(deviceConfig[CONFIG_WHITE_CHANNEL_GREEN].toInt(255), 255));
			_white_channel_blue = static_cast<uint8_t>(qMin(deviceConfig[CONFIG_WHITE_CHANNEL_BLUE].toInt(255), 255));

			DebugIf(_white_channel_calibration, _log, "White channel limit: %i (%.2f%), red: %i, green: %i, blue: %i", _white_channel_limit, _white_channel_limit_percent, _white_channel_red, _white_channel_green, _white_channel_blue);
		}
			break;

		case Adalight::LBAPA:
			Debug( _log, "Adalight driver uses LightBerry APA102 protocol");
			break;

		case Adalight::ADA:
			Debug( _log, "Adalight driver uses standard Adalight protocol");
			break;
		default:
			Error( _log, "Adalight driver - unsupported protocol");
			return false;
		}

		prepareHeader();
		isInitOK = true;
	}
	return isInitOK;
}

void LedDeviceAdalight::prepareHeader()
{
	// create ledBuffer
	uint totalLedCount = _ledCount;
	_bufferLength = static_cast<qint64>(HEADER_SIZE + _ledRGBCount);

	switch (_streamProtocol) {
	case Adalight::LBAPA:
	{
		const unsigned int startFrameSize = 4;
		const unsigned int bytesPerRGBLed = 4;
		const unsigned int endFrameSize = qMax<unsigned int>(((_ledCount + 15) / 16), bytesPerRGBLed);
		_bufferLength = HEADER_SIZE + (_ledCount * bytesPerRGBLed) + startFrameSize + endFrameSize;

		_ledBuffer.resize(static_cast<size_t>(_bufferLength), 0x00);

		// init constant data values
		for (uint iLed=1; iLed <= _ledCount; iLed++)
		{
			_ledBuffer[iLed*4+HEADER_SIZE] = 0xFF;
		}
	}
		break;

	case Adalight::AWA:
		_bufferLength += 7;
		[[clang::fallthrough]];
	case Adalight::ADA:
		[[clang::fallthrough]];
	default:
		totalLedCount -= 1;
		_ledBuffer.resize(static_cast<size_t>(_bufferLength), 0x00);
		break;
	}

	_ledBuffer[0] = 'A';
	if (_streamProtocol == Adalight::AWA )
	{
		_ledBuffer[1] = 'w';
		_ledBuffer[2] = _white_channel_calibration ? 'A' : 'a';
	}
	else
	{
		_ledBuffer[1] = 'd';
		_ledBuffer[2] = 'a';
	}

	qToBigEndian<quint16>(static_cast<quint16>(totalLedCount), &_ledBuffer[3]);
	_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum

	Debug( _log, "Adalight header for %d leds: %c%c%c 0x%02x 0x%02x 0x%02x", _ledCount,
		   _ledBuffer[0], _ledBuffer[1], _ledBuffer[2], _ledBuffer[3], _ledBuffer[4], _ledBuffer[5] );
}


int LedDeviceAdalight::write(const std::vector<ColorRgb> & ledValues)
{
	if (_ledCount != ledValues.size())
	{
		Warning(_log, "Adalight LED count has changed (old: %d, new: %d). Rebuilding header.", _ledCount, ledValues.size());
		_ledCount = static_cast<uint>(ledValues.size());
		_ledRGBCount = _ledCount * 3;
		prepareHeader();
	}

	if (_bufferLength >  static_cast<qint64>(_ledBuffer.size()))
	{
		Warning(_log, "Adalight buffer's size has changed. Skipping refresh.");
		return 0;
	}

	if (_streamProtocol == Adalight::LBAPA )
	{
		for (signed iLed=1; iLed<=static_cast<int>( _ledCount); iLed++)
		{
			const ColorRgb& rgb = ledValues[static_cast<size_t>(iLed-1)];
			_ledBuffer[static_cast<size_t>(iLed*4+7)] = rgb.red;
			_ledBuffer[static_cast<size_t>(iLed*4+8)] = rgb.green;
			_ledBuffer[static_cast<size_t>(iLed*4+9)] = rgb.blue;
		}
	}
	else
	{
		uint8_t* writer = _ledBuffer.data() + HEADER_SIZE;
		uint8_t* hasher = writer;

		memcpy(writer, ledValues.data(), ledValues.size() * sizeof(ColorRgb));
		writer += ledValues.size() * sizeof(ColorRgb);

		if (_streamProtocol == Adalight::AWA)
		{
			whiteChannelExtension(writer);

			uint16_t fletcher1 = 0, fletcher2 = 0;
			while (hasher < writer)
			{
				fletcher1 = (fletcher1 + *(hasher++)) % 255;
				fletcher2 = (fletcher2 + fletcher1) % 255;
			}
			*(writer++) = static_cast<uint8_t>(fletcher1);
			*(writer++) = static_cast<uint8_t>(fletcher2);
		}
		_bufferLength = writer - _ledBuffer.data();
	}

	int rc = writeBytes(_bufferLength, _ledBuffer.data());

	return rc;
}

void LedDeviceAdalight::whiteChannelExtension(uint8_t*& writer)
{
	if (_streamProtocol == Adalight::AWA && _white_channel_calibration)
	{
		*(writer++) = _white_channel_limit;
		*(writer++) = _white_channel_red;
		*(writer++) = _white_channel_green;
		*(writer++) = _white_channel_blue;
	}
}
