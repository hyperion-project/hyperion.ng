#include "LedDeviceTemplate.h"

LedDeviceTemplate::LedDeviceTemplate(const QJsonObject &deviceConfig)
	: LedDevice()
{
	_devConfig = deviceConfig;
	_deviceReady = false;
}

LedDeviceTemplate::~LedDeviceTemplate()
{
}

LedDevice* LedDeviceTemplate::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceTemplate(deviceConfig);
}

bool LedDeviceTemplate::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = LedDevice::init(deviceConfig);

	// Initiatiale LedDevice configuration and execution environment
	// ...
	if ( 0 /*Error during init*/)
	{
		//Build an errortext, illustrative
		QString errortext = QString ("Error message: %1").arg("errno/text");
		this->setInError(errortext);
		isInitOK = false;
	}

	return isInitOK;
}

int LedDeviceTemplate::open()
{
	int retval = -1;
	QString errortext;
	_deviceReady = false;

	// General initialisation and configuration of LedDevice
	if ( init(_devConfig) )
	{
		// Open/Start LedDevice based on configuration
		//...

		if ( false /*If opening failed*/ )
		{
			//Build an errortext, illustrative
			errortext = QString ("Failed to xxx. Error message: %1").arg("errno/text");
		}
		else
		{
			// Everything is OK -> enable device
			_deviceReady = true;
			setEnable(true);
			retval = 0;
		}

		// On error/exceptions, set LedDevice in error
		if ( retval < 0 )
		{
			this->setInError( errortext );
		}
	}
	return retval;
}

void LedDeviceTemplate::close()
{
	LedDevice::close();

	// LedDevice specific closing activites
	//...
}

int LedDeviceTemplate::write(const std::vector<ColorRgb> & ledValues)
{
	int retval = -1;

	//...

	return retval;
}


