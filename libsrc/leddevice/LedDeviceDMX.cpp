#include "LedDeviceDMX.h"
#include <QSerialPort>
#include <time.h>

LedDeviceDMX::LedDeviceDMX(const Json::Value &deviceConfig)
	: ProviderRs232(deviceConfig)
{
        std::string _dmxString = deviceConfig.get("dmxdevice", "invalid").asString();
	if (_dmxString == "raw")
	{
		_dmxDeviceType = 0;
		_dmxStart = 1;
		_dmxSlotsPerLed = 3;
	}
	else if (_dmxString == "McCrypt")
	{
		_dmxDeviceType = 1;
		_dmxStart = 1;
		_dmxSlotsPerLed = 4;
	}
	else
	{
		Error(_log, "unknown dmx device type %s", _dmxString.c_str());
	}
	Debug(_log, "_dmxString \"%s\", _dmxDeviceType %d", _dmxString.c_str(), _dmxDeviceType );
	_rs232Port.setStopBits(QSerialPort::TwoStop);
}

LedDevice* LedDeviceDMX::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceDMX(deviceConfig);
}

int LedDeviceDMX::write(const std::vector<ColorRgb> &ledValues)
{
	if ( (_ledBuffer.size() != _dmxChannelCount) || (_ledBuffer.size() == 0) )
	{
		_dmxLedCount  =  std::min(_ledCount, 512/_dmxSlotsPerLed);
		_dmxChannelCount  = 1 + _dmxSlotsPerLed * _dmxLedCount;

		Debug(_log, "_dmxStart %d, _dmxSlotsPerLed %d", _dmxStart, _dmxSlotsPerLed);
		Debug(_log, "_ledCount %d, _dmxLedCount %d, _dmxChannelCount %d", _ledCount, _dmxLedCount, _dmxChannelCount);

		_ledBuffer.resize(_dmxChannelCount, 0);
		_ledBuffer[0] = 0x00;	// NULL START code
	}

	switch (_dmxDeviceType) {
	case 0:
		memcpy(_ledBuffer.data()+1, ledValues.data(), _dmxChannelCount-1);
		break;
	case 1:
		int l =_dmxStart;
		for (int d=0; d<_dmxLedCount; d++)
		{
			_ledBuffer[l++] = ledValues[d].red;
			_ledBuffer[l++] = ledValues[d].green;
			_ledBuffer[l++] = ledValues[d].blue;
			_ledBuffer[l++] = 0xff;
		}
		break;
	}

	_rs232Port.setBreakEnabled(true);
	nanosleep((const struct timespec[]){{0, 176000L}}, NULL);	// 176 uSec break time
	_rs232Port.setBreakEnabled(false);
	nanosleep((const struct timespec[]){{0, 12000L}}, NULL);	// 176 uSec make after break time

#undef uberdebug
#ifdef uberdebug
	printf ("Writing %d bytes", _dmxChannelCount);
	for (unsigned int i=0; i < _dmxChannelCount; i++)
	{
		if (i%32 == 0) {
			printf ("\n%04x: ", i);
		}
		printf ("%02x ", _ledBuffer[i]);
	}
	printf ("\n");
#endif

	return writeBytes(_dmxChannelCount, _ledBuffer.data());
}

