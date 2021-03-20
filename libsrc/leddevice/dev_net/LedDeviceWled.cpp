// Local-Hyperion includes
#include "LedDeviceWled.h"

#include <utils/QStringUtils.h>
#include <utils/WaitTime.h>
#include <QThread>

#include <chrono>

// Constants
namespace {

const bool verbose = false;

// Configuration settings
const char CONFIG_ADDRESS[] = "host";
const char CONFIG_RESTORE_STATE[] = "restoreOriginalState";

// UDP elements
const quint16 STREAM_DEFAULT_PORT = 19446;

// WLED JSON-API elements
const int API_DEFAULT_PORT = -1; //Use default port per communication scheme

const char API_BASE_PATH[] = "/json/";
//const char API_PATH_INFO[] = "info";
const char API_PATH_STATE[] = "state";

// List of State Information
const char STATE_ON[] = "on";
const char STATE_VALUE_TRUE[] = "true";
const char STATE_VALUE_FALSE[] = "false";
const char STATE_LIVE[] = "live";

const int BRI_MAX = 255;

constexpr std::chrono::milliseconds DEFAULT_IDENTIFY_TIME{ 2000 };

} //End of constants

LedDeviceWled::LedDeviceWled(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig)
	  ,_restApi(nullptr)
	  ,_apiPort(API_DEFAULT_PORT)
{
}

LedDeviceWled::~LedDeviceWled()
{
	delete _restApi;
	_restApi = nullptr;
}

LedDevice* LedDeviceWled::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWled(deviceConfig);
}

bool LedDeviceWled::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise LedDevice sub-class, ProviderUdp::init will be executed later, if connectivity is defined
	if ( LedDevice::init(deviceConfig) )
	{
		// Initialise LedDevice configuration and execution environment
		int configuredLedCount = this->getLedCount();
		Debug(_log, "DeviceType   : %s", QSTRING_CSTR( this->getActiveDeviceType() ));
		Debug(_log, "LedCount     : %d", configuredLedCount);
		Debug(_log, "ColorOrder   : %s", QSTRING_CSTR( this->getColorOrder() ));
		Debug(_log, "LatchTime    : %d", this->getLatchTime());

		_isRestoreOrigState     = _devConfig[CONFIG_RESTORE_STATE].toBool(false);
		Debug(_log, "RestoreOrigState  : %d", _isRestoreOrigState);

		//Set hostname as per configuration
		QString hostName = deviceConfig[ CONFIG_ADDRESS ].toString();

		//If host not configured the init fails
		if ( hostName.isEmpty() )
		{
			this->setInError("No target hostname nor IP defined");
			return false;
		}
		else
		{
			QStringList addressparts = QStringUtils::split(hostName,":", QStringUtils::SplitBehavior::SkipEmptyParts);
			_hostname = addressparts[0];
			if ( addressparts.size() > 1 )
			{
				_apiPort = addressparts[1].toInt();
			}
			else
			{
				_apiPort = API_DEFAULT_PORT;
			}

			if ( initRestAPI( _hostname, _apiPort ) )
			{
				// Update configuration with hostname without port
				_devConfig["host"] = _hostname;
				_devConfig["port"] = STREAM_DEFAULT_PORT;

				isInitOK = ProviderUdp::init(_devConfig);
				Debug(_log, "Hostname/IP  : %s", QSTRING_CSTR( _hostname ));
				Debug(_log, "Port         : %d", _port);
			}
		}
	}
	return isInitOK;
}

bool LedDeviceWled::initRestAPI(const QString &hostname, int port)
{
	bool isInitOK = false;

	if ( _restApi == nullptr )
	{
		_restApi = new ProviderRestApi(hostname, port);
		_restApi->setBasePath( API_BASE_PATH );

		isInitOK = true;
	}
	return isInitOK;
}

QString LedDeviceWled::getOnOffRequest(bool isOn) const
{
	QString state = isOn ? STATE_VALUE_TRUE : STATE_VALUE_FALSE;
	return QString( "\"%1\":%2,\"%3\":%4" ).arg( STATE_ON, state).arg( STATE_LIVE, state);
}

QString LedDeviceWled::getBrightnessRequest(int bri) const
{
	return QString( "\"bri\":%1" ).arg(bri);
}

QString LedDeviceWled::getEffectRequest(int effect, int speed) const
{
	return QString( "\"seg\":{\"fx\":%1,\"sx\":%2}" ).arg(effect).arg(speed);
}

QString LedDeviceWled::getLorRequest(int lor) const
{
	return QString( "\"lor\":%1" ).arg(lor);
}

bool LedDeviceWled::sendStateUpdateRequest(const QString &request)
{
	bool rc = true;

	_restApi->setPath(API_PATH_STATE);

	httpResponse response1 = _restApi->put(QString("{%1}").arg(request));
	if ( response1.error() )
	{
		rc = false;
	}
	return rc;
}
bool LedDeviceWled::powerOn()
{
	bool on = false;
	if ( _isDeviceReady)
	{
		//Power-on WLED device
		_restApi->setPath(API_PATH_STATE);

		httpResponse response = _restApi->put(QString("{%1,%2}").arg(getOnOffRequest(true)).arg(getBrightnessRequest(BRI_MAX)));
		if ( response.error() )
		{
			QString errorReason = QString("Power-on request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError ( errorReason );
			on = false;
		}
		else
		{
			on = true;
		}
	}
	return on;
}

bool LedDeviceWled::powerOff()
{
	bool off = true;
	if ( _isDeviceReady)
	{
		// Write a final "Black" to have a defined outcome
		writeBlack();

		//Power-off the WLED device physically
		_restApi->setPath(API_PATH_STATE);
		httpResponse response = _restApi->put(QString("{%1}").arg(getOnOffRequest(false)));
		if ( response.error() )
		{
			QString errorReason = QString("Power-off request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError ( errorReason );
			off = false;
		}
	}
	return off;
}

bool LedDeviceWled::storeState()
{
	bool rc = true;

	if ( _isRestoreOrigState )
	{
		_restApi->setPath(API_PATH_STATE);

		httpResponse response = _restApi->get();
		if ( response.error() )
		{
			QString errorReason = QString("Storing device state failed with error: '%1'").arg(response.getErrorReason());
			setInError(errorReason);
			rc = false;
		}
		else
		{
			_originalStateProperties = response.getBody().object();
			DebugIf(verbose, _log, "state: [%s]", QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData() );
		}
	}

	return rc;
}

bool LedDeviceWled::restoreState()
{
	bool rc = true;

	if ( _isRestoreOrigState )
	{
		//powerOff();
		_restApi->setPath(API_PATH_STATE);

		_originalStateProperties[STATE_LIVE] = false;

		httpResponse response = _restApi->put(QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData());

		if ( response.error() )
		{
			Warning (_log, "%s restoring state failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		}
	}

	return rc;
}

QJsonObject LedDeviceWled::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;
	devicesDiscovered.insert("devices", deviceList);
	DebugIf(verbose, _log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return devicesDiscovered;
}

QJsonObject LedDeviceWled::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	QJsonObject properties;

	QString hostName = params["host"].toString("");

	if ( !hostName.isEmpty() )
	{
		QString filter = params["filter"].toString("");

		// Resolve hostname and port (or use default API port)
		QStringList addressparts = QStringUtils::split(hostName,":", QStringUtils::SplitBehavior::SkipEmptyParts);
		QString apiHost = addressparts[0];
		int apiPort;

		if ( addressparts.size() > 1)
		{
			apiPort = addressparts[1].toInt();
		}
		else
		{
			apiPort   = API_DEFAULT_PORT;
		}

		initRestAPI(apiHost, apiPort);
		_restApi->setPath(filter);

		httpResponse response = _restApi->get();
		if ( response.error() )
		{
			Warning (_log, "%s get properties failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		}

		properties.insert("properties", response.getBody().object());

		DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	}
	return properties;
}

void LedDeviceWled::identify(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QString hostName = params["host"].toString("");

	if ( !hostName.isEmpty() )
	{
		// Resolve hostname and port (or use default API port)
		QStringList addressparts = QStringUtils::split(hostName,":", QStringUtils::SplitBehavior::SkipEmptyParts);
		QString apiHost = addressparts[0];
		int apiPort;

		if ( addressparts.size() > 1)
		{
			apiPort = addressparts[1].toInt();
		}
		else
		{
			apiPort   = API_DEFAULT_PORT;
		}

		initRestAPI(apiHost, apiPort);

		_isRestoreOrigState = true;
		storeState();

		QString request = getOnOffRequest(true) + "," + getLorRequest(1) + "," + getEffectRequest(25);
		sendStateUpdateRequest(request);

		wait(DEFAULT_IDENTIFY_TIME);

		restoreState();
	}
}

int LedDeviceWled::write(const std::vector<ColorRgb> &ledValues)
{
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes( _ledRGBCount, dataPtr);
}
