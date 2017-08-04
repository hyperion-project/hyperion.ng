#include "LedDeviceDMX.h"
#include <QSerialPort>
#include <time.h>

LedDeviceDMX::LedDeviceDMX(const QJsonObject &deviceConfig)
	: ProviderRs232()
	, _dmxDeviceType(0)
	, _dmxStart(1)
	, _dmxSlotsPerLed(3)
	, _dmxLedCount(0)
	, _dmxChannelCount(0)
{
	_deviceReady = init(deviceConfig);
}

bool LedDeviceDMX::init(const QJsonObject &deviceConfig)
{
	ProviderRs232::init(deviceConfig);

	QString dmxString = deviceConfig["dmxdevice"].toString("invalid");
	if (dmxString == "raw")
	{
		_dmxDeviceType = 0;
		_dmxStart = 1;
		_dmxSlotsPerLed = 3;
	}
	else if (dmxString == "McCrypt")
	{
		_dmxDeviceType = 1;
		_dmxStart = 1;
		_dmxSlotsPerLed = 4;
	}
	else
	{
		Error(_log, "unknown dmx device type %s", QSTRING_CSTR(dmxString));
	}

	Debug(_log, "_dmxString \"%s\", _dmxDeviceType %d", QSTRING_CSTR(dmxString), _dmxDeviceType );
	_rs232Port.setStopBits(QSerialPort::TwoStop);
	
	_dmxLedCount  =  qMin(_ledCount, 512/_dmxSlotsPerLed);
	_dmxChannelCount  = 1 + _dmxSlotsPerLed * _dmxLedCount;

	Debug(_log, "_dmxStart %d, _dmxSlotsPerLed %d", _dmxStart, _dmxSlotsPerLed);
	Debug(_log, "_ledCount %d, _dmxLedCount %d, _dmxChannelCount %d", _ledCount, _dmxLedCount, _dmxChannelCount);

	_ledBuffer.resize(_dmxChannelCount, 0);
	_ledBuffer[0] = 0x00;	// NULL START code

	return true;
}

LedDevice* LedDeviceDMX::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceDMX(deviceConfig);
}

int LedDeviceDMX::write(const std::vector<ColorRgb> &ledValues)
{
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
			_ledBuffer[l++] = 255;
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
