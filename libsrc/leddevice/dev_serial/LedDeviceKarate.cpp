// hyperion local includes
#include "LedDeviceKarate.h"

namespace {
	const bool verbose = false;
} //End of constants

LedDeviceKarate::LedDeviceKarate(const QJsonObject& deviceConfig)
	: ProviderRs232(deviceConfig)
{
}

LedDevice* LedDeviceKarate::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceKarate(deviceConfig);
}

bool LedDeviceKarate::init(const QJsonObject& deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if (ProviderRs232::init(deviceConfig))
	{
		if (_ledCount != 8 && _ledCount != 16)
		{
			QString errortext = QString("%1 channels configured. This should always be 8 or 16!").arg(_ledCount);
			this->setInError(errortext);
			isInitOK = false;
		}
		else
		{
			_ledBuffer.resize(4 + _ledCount * 3); // 4-byte header, 3 RGB values
			_ledBuffer[0] = 0xAA;				  // Startbyte
			_ledBuffer[1] = 0x12;				  // Send all Channels in Batch
			_ledBuffer[2] = 0x00;				  // Checksum
			_ledBuffer[3] = _ledCount * 3;        // Number of Databytes send

			Debug(_log, "Karatelight header for %d leds: 0x%02x 0x%02x 0x%02x 0x%02x", _ledCount,
				_ledBuffer[0], _ledBuffer[1], _ledBuffer[2], _ledBuffer[3]);

			isInitOK = true;
		}
	}
	return isInitOK;
}

int LedDeviceKarate::write(const std::vector<ColorRgb>& ledValues)
{
	for (signed iLed = 0; iLed < static_cast<int>(_ledCount); iLed++)
	{
		const ColorRgb& rgb = ledValues[iLed];
		_ledBuffer[iLed * 3 + 4] = rgb.green;
		_ledBuffer[iLed * 3 + 5] = rgb.blue;
		_ledBuffer[iLed * 3 + 6] = rgb.red;
	}

	// Calc Checksum
	_ledBuffer[2] = _ledBuffer[0] ^ _ledBuffer[1];
	for (unsigned int i = 3; i < _ledBuffer.size(); i++)
		_ledBuffer[2] ^= _ledBuffer[i];

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

QJsonObject LedDeviceKarate::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());
	QJsonObject properties;

	QString serialPort = params["serialPort"].toString("");

	QJsonObject propertiesDetails;
	QJsonArray possibleLedCounts = { 16, 8 };
	propertiesDetails.insert("ledCount", possibleLedCounts);

	properties.insert("properties", propertiesDetails);

	DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData());
	return properties;
}
