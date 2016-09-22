#include "LedDeviceAdalight.h"

LedDeviceAdalight::LedDeviceAdalight(const Json::Value &deviceConfig)
	: ProviderRs232(deviceConfig)
	, _timer()
{
	// setup the timer
	_timer.setSingleShot(false);
	_timer.setInterval(5000);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(rewriteLeds()));

	// start the timer
	_timer.start();
}

LedDevice* LedDeviceAdalight::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceAdalight(deviceConfig);
}

int LedDeviceAdalight::write(const std::vector<ColorRgb> & ledValues)
{
	if (_ledBuffer.size() == 0)
	{
		_ledBuffer.resize(6 + 3*ledValues.size());
		_ledBuffer[0] = 'A';
		_ledBuffer[1] = 'd';
		_ledBuffer[2] = 'a';
		_ledBuffer[3] = ((ledValues.size() - 1) >> 8) & 0xFF; // LED count high byte
		_ledBuffer[4] = (ledValues.size() - 1) & 0xFF;        // LED count low byte
		_ledBuffer[5] = _ledBuffer[3] ^ _ledBuffer[4] ^ 0x55; // Checksum
                Debug( _log, "Adalight header for %d leds: %c%c%c 0x%02x 0x%02x 0x%02x",
			ledValues.size(),
			_ledBuffer[0],
			_ledBuffer[1],
			_ledBuffer[2],
			_ledBuffer[3],
			_ledBuffer[4],
			_ledBuffer[5]
		);
	}

	// restart the timer
	_timer.start();

	// write data
	memcpy(6 + _ledBuffer.data(), ledValues.data(), ledValues.size() * 3);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

void LedDeviceAdalight::rewriteLeds()
{
	writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
