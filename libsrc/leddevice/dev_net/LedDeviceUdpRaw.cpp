#include "LedDeviceUdpRaw.h"

#include <utils/NetUtils.h>

// Constants
namespace {

const bool verbose = false;

const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";
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
	bool isInitOK {false};

	if ( ProviderUdp::init(deviceConfig) )
	{
		if (this->getLedCount() > UDP_MAX_LED_NUM)
		{
			QString errorReason = QString("Device type %1 can only be run with maximum %2 LEDs for streaming protocol = UDP-RAW!").arg(this->getActiveDeviceType()).arg(UDP_MAX_LED_NUM);
			this->setInError ( errorReason );
		}
		else
		{
			_hostName = deviceConfig[ CONFIG_HOST ].toString();
			_port = deviceConfig[CONFIG_PORT].toInt(RAW_DEFAULT_PORT);

			Debug(_log, "Hostname/IP       : %s", QSTRING_CSTR(_hostName) );
			Debug(_log, "Port              : %d", _port );

			isInitOK = true;
		}
	}
	return isInitOK;
}

int LedDeviceUdpRaw::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	{
		if (ProviderUdp::open() == 0)
		{
			// Everything is OK, device is ready
			_isDeviceReady = true;
			retval = 0;
		}
	}
	return retval;
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

	Info(_log, "Get properties for %s", QSTRING_CSTR(_activeDeviceType));

	QJsonObject propertiesDetails;
	propertiesDetails.insert("maxLedCount", UDP_MAX_LED_NUM);

	properties.insert("properties", propertiesDetails);

	DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return properties;
}
