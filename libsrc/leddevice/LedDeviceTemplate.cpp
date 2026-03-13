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
	// Initialise sub-class
	if (!LedDevice::init(deviceConfig))
	{
		return false;
	}

	// Initialise LedDevice configuration and execution environment
	// ...
	if ( false /*Error during init*/)
	{
		//Build an errortext, illustrative
		QString errortext = QString ("Error message: %1").arg("errno/text");
		this->setInError(errortext);
		return false;
	}

	return true;
}

int LedDeviceTemplate::open()
{
	QString errortext;
	_isDeviceReady = false;

	// Try to open the LedDevice
	//...

	if ( false /*If opening failed*/ )
	{
		//Build an errortext, illustrative
		errortext = QString ("Failed to xxx. Error message: %1").arg("errno/text");
		return -1;
	}

	// Everything is OK, device is ready
	_isDeviceReady = true;

	return 0;
}

int LedDeviceTemplate::close()
{
	// LedDevice specific closing activities
	//...

	_isDeviceReady = false;

#if 0
	// Test, if device requires closing
	if ( true /*If device is still open*/ )
	{
		// Close device
		// Everything is OK -> device is closed
	}
#endif
	return 0;
}

int LedDeviceTemplate::write(const QVector<ColorRgb> & ledValues)
{
	//...

	return 0;
}
