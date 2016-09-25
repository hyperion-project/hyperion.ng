#include "LedDeviceDMX.h"
#include <QSerialPort>
#include <time.h>

LedDeviceDMX::LedDeviceDMX(const Json::Value &deviceConfig)
	: ProviderRs232(deviceConfig)
{
	_rs232Port.setStopBits(QSerialPort::TwoStop);
}

LedDevice* LedDeviceDMX::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceDMX(deviceConfig);
}

int LedDeviceDMX::write(const std::vector<ColorRgb> &ledValues)
{
	uint8_t _dmxChannelCount  = 1 + std::min(3 * _ledCount, 512);
 
	if (_ledBuffer.size() != _dmxChannelCount)
	{
		_ledBuffer.resize(_dmxChannelCount, 0);
		_ledBuffer[0] = 0x00;	// NULL START code
	}

	memcpy(_ledBuffer.data()+1, ledValues.data(), _dmxChannelCount-1);

	_rs232Port.setBreakEnabled(true);
	nanosleep((const struct timespec[]){{0, 176000L}}, NULL);	// 176 uSec break time
	_rs232Port.setBreakEnabled(false);
	nanosleep((const struct timespec[]){{0, 12000L}}, NULL);	// 176 uSec make after break time

	return writeBytes(_dmxChannelCount, _ledBuffer.data());
}
