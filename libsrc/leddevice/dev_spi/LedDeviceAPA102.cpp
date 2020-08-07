#include "LedDeviceAPA102.h"

LedDeviceAPA102::LedDeviceAPA102(const QJsonObject &deviceConfig)
	: ProviderSpi(deviceConfig)
{
}

LedDevice* LedDeviceAPA102::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceAPA102(deviceConfig);
}

bool LedDeviceAPA102::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( ProviderSpi::init(deviceConfig) )
	{

		const unsigned int startFrameSize = 4;
		const unsigned int endFrameSize = qMax<unsigned int>(((_ledCount + 15) / 16), 4);
		const unsigned int APAbufferSize = (_ledCount * 4) + startFrameSize + endFrameSize;

		_ledBuffer.resize(APAbufferSize, 0xFF);
		_ledBuffer[0] = 0x00;
		_ledBuffer[1] = 0x00;
		_ledBuffer[2] = 0x00;
		_ledBuffer[3] = 0x00;

		isInitOK = true;

	}
	return isInitOK;
}

int LedDeviceAPA102::write(const std::vector<ColorRgb> &ledValues)
{
	for (signed iLed=0; iLed < static_cast<int>( _ledCount); ++iLed) {
		const ColorRgb& rgb = ledValues[iLed];
		_ledBuffer[4+iLed*4]   = 0xFF;
		_ledBuffer[4+iLed*4+1] = rgb.red;
		_ledBuffer[4+iLed*4+2] = rgb.green;
		_ledBuffer[4+iLed*4+3] = rgb.blue;
	}

	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
