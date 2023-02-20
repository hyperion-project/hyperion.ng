#include "LedDeviceTemplate.h"

LedDeviceTemplate::LedDeviceTemplate(const QJsonObject & /*deviceConfig*/)
{
}

LedDevice* LedDeviceTemplate::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceTemplate(deviceConfig);
}

bool LedDeviceTemplate::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		// Initialise LedDevice configuration and execution environment
		// ...
		if ( false /*Error during init*/)
		{
			//Build an errortext, illustrative
			QString errortext = QString ("Error message: %1").arg("errno/text");
			this->setInError(errortext);
			isInitOK = false;
		}
		else
		{
			isInitOK = true;
		}
	}
	return isInitOK;
}

int LedDeviceTemplate::open()
{
	int retval = -1;
	QString errortext;
	_isDeviceReady = false;

	// Try to open the LedDevice
	//...

	if ( false /*If opening failed*/ )
	{
		//Build an errortext, illustrative
		errortext = QString ("Failed to xxx. Error message: %1").arg("errno/text");
	}
	else
	{
		// Everything is OK, device is ready
		_isDeviceReady = true;
		retval = 0;
	}

	// On error/exceptions, set LedDevice in error
	if ( false /* retval < 0*/ )
	{
		this->setInError( errortext );
	}
	return retval;
}

int LedDeviceTemplate::close()
{
	// LedDevice specific closing activities
	//...
	int retval = 0;
	_isDeviceReady = false;

#if 0
	// Test, if device requires closing
	if ( true /*If device is still open*/ )
	{
		// Close device
		// Everything is OK -> device is closed
	}
#endif
	return retval;
}

int LedDeviceTemplate::write(const std::vector<ColorRgb> & ledValues)
{
	int retval = -1;

	//...

	return retval;
}
