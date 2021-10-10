#include "LedDeviceUdpRaw.h"

// Constants
namespace {

const bool verbose = false;

const ushort RAW_DEFAULT_PORT=5568;
const int UDP_MAX_LED_NUM = 490;

} //End of constants

LedDeviceUdpRaw::LedDeviceUdpRaw(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
{
}

LedDevice* LedDeviceUdpRaw::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceUdpRaw(deviceConfig);
}

bool LedDeviceUdpRaw::init(const QJsonObject &deviceConfig)
{
	_port = RAW_DEFAULT_PORT;

	bool isInitOK = false;
	if ( LedDevice::init(deviceConfig) )
	{
		// Initialise LedDevice configuration and execution environment
		int configuredLedCount = this->getLedCount();
		Debug(_log, "DeviceType   : %s", QSTRING_CSTR( this->getActiveDeviceType() ));
		Debug(_log, "LedCount     : %d", configuredLedCount);
		Debug(_log, "ColorOrder   : %s", QSTRING_CSTR( this->getColorOrder() ));
		Debug(_log, "LatchTime    : %d", this->getLatchTime());

		if (configuredLedCount > UDP_MAX_LED_NUM)
		{
			QString errorReason = QString("Device type %1 can only be run with maximum %2 LEDs!").arg(this->getActiveDeviceType()).arg(UDP_MAX_LED_NUM);
			this->setInError ( errorReason );
			isInitOK = false;
		}
		else
		{
			// Initialise sub-class
			isInitOK = ProviderUdp::init(deviceConfig);
		}
	}
	return isInitOK;
}

int LedDeviceUdpRaw::write(const std::vector<ColorRgb> &ledValues)
{
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes(_ledRGBCount, dataPtr);
}

QJsonObject LedDeviceUdpRaw::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	QJsonObject properties;

	QJsonObject propertiesDetails;
	propertiesDetails.insert("maxLedCount", UDP_MAX_LED_NUM);

	properties.insert("properties", propertiesDetails);

	DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return properties;
}
