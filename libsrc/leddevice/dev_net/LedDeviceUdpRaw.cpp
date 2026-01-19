#include "LedDeviceUdpRaw.h"

#include <utils/NetUtils.h>
#include "utils/RgbToRgbw.h"

// Constants
namespace {
const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";
const ushort RAW_DEFAULT_PORT=5568;
} //End of constants

LedDeviceUdpRaw::LedDeviceUdpRaw(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
	, _whiteAlgorithm(RGBW::WhiteAlgorithm::INVALID)
{
}

LedDevice* LedDeviceUdpRaw::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpRaw(deviceConfig);
}

bool LedDeviceUdpRaw::init(const QJsonObject &deviceConfig)
{
	if (!ProviderUdp::init(deviceConfig))
	{
		return false;
	}

	_hostName = deviceConfig[ CONFIG_HOST ].toString();
	_port = deviceConfig[CONFIG_PORT].toInt(RAW_DEFAULT_PORT);

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

	int maxLedCount = (_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF) ? UdpRaw::MAX_LED_NUM_RGB : UdpRaw::MAX_LED_NUM_RGBW;
	if (this->getLedCount() > maxLedCount)
	{
		QString errorReason = QString("Device type %1 can only be run with maximum %2 LEDs for streaming protocol = UDP-RAW and LED type = %3!").arg(this->getActiveDeviceType()).arg(maxLedCount).arg((_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF) ? "RGB" : "RGBW");
		this->setInError ( errorReason );
		return false;
	}

	return true;
}

int LedDeviceUdpRaw::open()
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

int LedDeviceUdpRaw::write(const QVector<ColorRgb> &ledValues)
{
	int channelCount;
	const uint8_t* dataPtr = nullptr;

	if (_whiteAlgorithm == RGBW::WhiteAlgorithm::WHITE_OFF)
	{
		channelCount = _ledRGBCount; // 1 channel for every R,G,B value
		dataPtr =reinterpret_cast<const uint8_t*>(ledValues.data());
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
		dataPtr = reinterpret_cast<const uint8_t*>(rgbwLedValues.data());
	}

	return writeBytes(channelCount, dataPtr);
}

QJsonObject LedDeviceUdpRaw::getProperties(const QJsonObject& params)
{
	QJsonObject properties;

	Info(_log, "Get properties for %s", QSTRING_CSTR(_activeDeviceType));

	QJsonObject propertiesDetails;
	QJsonObject maxLedNumberInfo;
	maxLedNumberInfo.insert("rgb", UdpRaw::MAX_LED_NUM_RGB);
	maxLedNumberInfo.insert("rgbw", UdpRaw::MAX_LED_NUM_RGBW);
	propertiesDetails.insert("maxLedCount", maxLedNumberInfo);

	properties.insert("properties", propertiesDetails);

	return properties;
}
