#include "LedDeviceAdalightApa102.h"

LedDeviceAdalightApa102::LedDeviceAdalightApa102(const Json::Value &deviceConfig)
	: ProviderRs232()
{
	_deviceReady = setConfig(deviceConfig);
}

LedDevice* LedDeviceAdalightApa102::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceAdalightApa102(deviceConfig);
}

bool LedDeviceAdalightApa102::setConfig(const Json::Value &deviceConfig)
{
	ProviderRs232::setConfig(deviceConfig);

	const unsigned int startFrameSize = 4;
	const unsigned int endFrameSize = std::max<unsigned int>(((_ledCount + 15) / 16), 4);
	const unsigned int mLedCount = (_ledCount * 4) + startFrameSize + endFrameSize;
	if(_ledBuffer.size() != mLedCount+6)
	{
		_ledBuffer.resize(mLedCount+6, 0x00);
		_ledBuffer[0] = 'A';
		_ledBuffer[1] = 'd';
		_ledBuffer[2] = 'a';
		_ledBuffer[3] = (((unsigned int)(_ledCount)) >> 8) & 0xFF; // LED count high byte
		_ledBuffer[4] = ((unsigned int)(_ledCount)) & 0xFF;        // LED count low byte
		_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum
		Debug( _log, "Adalight header for %d leds: %c%c%c 0x%02x 0x%02x 0x%02x", _ledCount,
			_ledBuffer[0], _ledBuffer[1], _ledBuffer[2], _ledBuffer[3], _ledBuffer[4], _ledBuffer[5] );
	}

	return true;
}

//comparing to ws2801 adalight, the following changes were needed:
// 1- differnt data frame (4 bytes instead of 3)
// 2 - in order to accomodate point 1 above, number of leds sent to adalight is increased by 1/3rd
int LedDeviceAdalightApa102::write(const std::vector<ColorRgb> & ledValues)
{
	for (signed iLed=1; iLed<=_ledCount; iLed++)
	{
		const ColorRgb& rgb = ledValues[iLed-1];
		_ledBuffer[iLed*4+6]   = 0xFF;
		_ledBuffer[iLed*4+1+6] = rgb.red;
		_ledBuffer[iLed*4+2+6] = rgb.green;
		_ledBuffer[iLed*4+3+6] = rgb.blue;
	}

	// write data
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

