#include "LedDeviceAdalight.h"

LedDeviceAdalight::LedDeviceAdalight(const QJsonObject &deviceConfig)
	: ProviderRs232()
	, _headerSize(6)
	, _ligthBerryAPA102Mode(false)
{
	_deviceReady = init(deviceConfig);
	connect(this,SIGNAL(receivedData(QByteArray)),this,SLOT(receivedData(QByteArray)));
}

LedDevice* LedDeviceAdalight::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAdalight(deviceConfig);
}

bool LedDeviceAdalight::init(const QJsonObject &deviceConfig)
{
	ProviderRs232::init(deviceConfig);
	_ligthBerryAPA102Mode = deviceConfig["lightberry_apa102_mode"].toBool(false);

	// create ledBuffer
	unsigned int totalLedCount = _ledCount;

	if (_ligthBerryAPA102Mode)
	{
		const unsigned int startFrameSize = 4;
		const unsigned int bytesPerRGBLed = 4;
		const unsigned int endFrameSize = qMax<unsigned int>(((_ledCount + 15) / 16), bytesPerRGBLed);
		_ledBuffer.resize(_headerSize + (_ledCount * bytesPerRGBLed) + startFrameSize + endFrameSize, 0x00);
		
		// init constant data values
		for (signed iLed=1; iLed<=_ledCount; iLed++)
		{
			_ledBuffer[iLed*4+_headerSize] = 0xFF;
		}
		Debug( _log, "Adalight driver with activated LightBerry APA102 mode");
	}
	else
	{
		totalLedCount -= 1;
		_ledBuffer.resize(_headerSize + _ledRGBCount, 0x00);
	}

	_ledBuffer[0] = 'A';
	_ledBuffer[1] = 'd';
	_ledBuffer[2] = 'a';
	_ledBuffer[3] = (totalLedCount >> 8) & 0xFF; // LED count high byte
	_ledBuffer[4] = totalLedCount & 0xFF;        // LED count low byte
	_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum

	Debug( _log, "Adalight header for %d leds: %c%c%c 0x%02x 0x%02x 0x%02x", _ledCount,
			_ledBuffer[0], _ledBuffer[1], _ledBuffer[2], _ledBuffer[3], _ledBuffer[4], _ledBuffer[5] );

	return true;
}

int LedDeviceAdalight::write(const std::vector<ColorRgb> & ledValues)
{
	if(_ligthBerryAPA102Mode)
	{
		for (signed iLed=1; iLed<=_ledCount; iLed++)
		{
			const ColorRgb& rgb = ledValues[iLed-1];
			_ledBuffer[iLed*4+7] = rgb.red;
			_ledBuffer[iLed*4+8] = rgb.green;
			_ledBuffer[iLed*4+9] = rgb.blue;
		}
	}
	else
	{
		memcpy(_headerSize + _ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	}
	
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceAdalight::receivedData(QByteArray data)
{
	Debug(_log, ">>received %d bytes data", data.size());
}
