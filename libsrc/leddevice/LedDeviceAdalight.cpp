#include "LedDeviceAdalight.h"

LedDeviceAdalight::LedDeviceAdalight(const QJsonObject &deviceConfig)
	: ProviderRs232()
	, _ligthBerryAPA102Mode(false)
{
	_deviceReady = init(deviceConfig);
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
	unsigned int bufferSize    = 6;          // 6 bytes header
	unsigned int totalLedCount = _ledCount;

	if (_ligthBerryAPA102Mode)
	{
		const unsigned int startFrameSize = 4;
		const unsigned int bytesPerRGBLed = 4;
		const unsigned int endFrameSize = std::max<unsigned int>(((_ledCount + 15) / 16), bytesPerRGBLed);
		bufferSize += (_ledCount * bytesPerRGBLed) + startFrameSize + endFrameSize ;
	}
	else
	{
		totalLedCount -= 1;
		bufferSize += _ledRGBCount;
	}

	_ledBuffer.resize(bufferSize, 0x00);
	_ledBuffer[0] = 'A';
	_ledBuffer[1] = 'd';
	_ledBuffer[2] = 'a';
	_ledBuffer[3] = (((unsigned int)(totalLedCount)) >> 8) & 0xFF; // LED count high byte
	_ledBuffer[4] = ((unsigned int)(totalLedCount)) & 0xFF;        // LED count low byte
	_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum

	Debug( _log, "Adalight header for %d leds: %c%c%c 0x%02x 0x%02x 0x%02x", _ledCount,
			_ledBuffer[0], _ledBuffer[1], _ledBuffer[2], _ledBuffer[3], _ledBuffer[4], _ledBuffer[5]
		);

	return true;
}

int LedDeviceAdalight::write(const std::vector<ColorRgb> & ledValues)
{
	if(_ligthBerryAPA102Mode)
	{
		for (signed iLed=1; iLed<=_ledCount; iLed++)
		{
			const ColorRgb& rgb = ledValues[iLed-1];
			_ledBuffer[iLed*4+6]   = 0xFF;
			_ledBuffer[iLed*4+1+6] = rgb.red;
			_ledBuffer[iLed*4+2+6] = rgb.green;
			_ledBuffer[iLed*4+3+6] = rgb.blue;
		}
	}
	else
	{
		memcpy(6 + _ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	}
	
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

