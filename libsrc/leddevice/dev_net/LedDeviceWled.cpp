// Local-Hyperion includes
#include "LedDeviceWled.h"

// ssdp discover
#include <ssdp/SSDPDiscover.h>

// Configuration settings
static const char CONFIG_ADDRESS[] = "host";
static const char CONFIG_PORT[] = "port";

// UDP elements
const quint16 STREAM_DEFAULT_PORT = 19446;

// WLED JSON-API elements
static const int API_DEFAULT_PORT = -1; //Use default port per communication scheme

static const char API_BASE_PATH[] = "/json/";
static const char API_PATH_INFO[] = "info";
static const char API_PATH_STATE[] = "state";

// List of State Information
static const char STATE_ON[] = "on";
static const char STATE_VALUE_TRUE[] = "true";
static const char STATE_VALUE_FALSE[] = "false";

LedDeviceWled::LedDeviceWled(const QJsonObject &deviceConfig)
	: ProviderUdp()
	  ,_restApi(nullptr)
	  ,_apiPort(API_DEFAULT_PORT)
	  ,_streamPort(STREAM_DEFAULT_PORT)
{
	_devConfig = deviceConfig;
	_isDeviceReady = false;

	_activeDeviceType = deviceConfig["type"].toString("UNSPECIFIED").toLower();
}

LedDeviceWled::~LedDeviceWled()
{
	if ( _restApi != nullptr )
	{
		delete _restApi;
		_restApi = nullptr;
	}
}

LedDevice* LedDeviceWled::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWled(deviceConfig);
}

bool LedDeviceWled::init(const QJsonObject &deviceConfig)
{
	Debug(_log, "");
	bool isInitOK = false;

	// Initialise LedDevice sub-class, ProviderUdp::init will be executed later, if connectivity is defined
	if ( LedDevice::init(deviceConfig) )
	{
		// Initialise LedDevice configuration and execution environment
		uint configuredLedCount = this->getLedCount();
		Debug(_log, "DeviceType   : %s", QSTRING_CSTR( this->getActiveDeviceType() ));
		Debug(_log, "LedCount     : %u", configuredLedCount);
		Debug(_log, "ColorOrder   : %s", QSTRING_CSTR( this->getColorOrder() ));
		Debug(_log, "LatchTime    : %d", this->getLatchTime());

		//Set hostname as per configuration and_defaultHost default port
		QString address = deviceConfig[ CONFIG_ADDRESS ].toString();
		_port = deviceConfig[ CONFIG_PORT ].toInt();

		//If host not configured the init fails
		if ( address.isEmpty() )
		{
			this->setInError("No target hostname nor IP defined");
			return false;
		}
		else
		{
			QStringList addressparts = address.split(":", QString::SkipEmptyParts);
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

				isInitOK = ProviderUdp::init(_devConfig);
				Debug(_log, "Hostname/IP  : %s", QSTRING_CSTR( _hostname ));
				Debug(_log, "Port         : %d", _port);
			}
		}
	}
	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

bool LedDeviceWled::initRestAPI(const QString &hostname, const int port )
{
	Debug(_log, "");
	bool isInitOK = false;

	if ( _restApi == nullptr )
	{
		_restApi = new ProviderRestApi(hostname, port);
		_restApi->setBasePath( API_BASE_PATH );

		isInitOK = true;
	}

	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

QString LedDeviceWled::getOnOffRequest (bool isOn ) const
{
	QString state = isOn ? STATE_VALUE_TRUE : STATE_VALUE_FALSE;
	return QString( "{\"%1\":%2}}" ).arg( STATE_ON, state);
}

bool LedDeviceWled::powerOn()
{
	Debug(_log, "");
	bool on = true;
	if ( _isDeviceReady)
	{
		//Power-on WLED device
		_restApi->setPath(API_PATH_STATE);
		httpResponse response = _restApi->put(getOnOffRequest(true));
		if ( response.error() )
		{
			this->setInError ( response.getErrorReason() );
			on = false;
		}
	}
	return on;
}

bool LedDeviceWled::powerOff()
{
	Debug(_log, "");
	bool off = true;
	if ( _isDeviceReady)
	{
		// Write a final "Black" to have a defined outcome
		writeBlack();

		//Power-off the WLED device physically
		_restApi->setPath(API_PATH_STATE);
		httpResponse response = _restApi->put(getOnOffRequest(false));
		if ( response.error() )
		{
			this->setInError ( response.getErrorReason() );
			off = false;
		}
	}
	return off;
}

QJsonObject LedDeviceWled::discover()
{
	Debug(_log, "");

	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	// Discover WLED Devices
	SSDPDiscover discover;

	// TODO: Update filter to device specifics or change discovery method to mDNS

	//QString searchTargetFilter = QString("%1|%2").arg(SSDP_CANVAS, SSDP_LIGHTPANELS);
	QString searchTargetFilter = QString("*");

	discover.setSearchFilter(searchTargetFilter, "ST");
	QString searchTarget = "ssdp:all";

	if ( discover.discoverServices(searchTarget) > 0 )
	{
		deviceList = discover.getServicesDiscoveredJson();
	}

	devicesDiscovered.insert("devices", deviceList);
	Debug(_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return devicesDiscovered;
}

QJsonObject LedDeviceWled::getProperties(const QJsonObject& params)
{
	Debug(_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	QJsonObject properties;

	// Get Nanoleaf device properties
	QString host = params["host"].toString("");
	if ( !host.isEmpty() )
	{
		QString filter = params["filter"].toString("");

		// Resolve hostname and port (or use default API port)
		QStringList addressparts = host.split(":", QString::SkipEmptyParts);
		QString apiHost = addressparts[0];
		int apiPort;

		if ( addressparts.size() > 1)
			apiPort = addressparts[1].toInt();
		else
			apiPort   = API_DEFAULT_PORT;

		if ( filter.startsWith("/") )
			filter.remove(0,1);

		initRestAPI(apiHost, apiPort);
		_restApi->setPath(API_PATH_INFO);

		// Perform request
		// TODO: Check, if filter is supported
		httpResponse response = _restApi->put(filter);
		if ( response.error() )
		{
			Warning (_log, "%s get properties failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		}

		properties.insert("properties", response.getBody().object());

		Debug(_log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	}
	return properties;
}

void LedDeviceWled::identify(const QJsonObject& params)
{
	Debug(_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	QJsonObject properties;

	// Get Nanoleaf device properties
	QString host = params["host"].toString("");
	if ( !host.isEmpty() )
	{
		// Resolve hostname and port (or use default API port)
		QStringList addressparts = host.split(":", QString::SkipEmptyParts);
		QString apiHost = addressparts[0];
		int apiPort;

		if ( addressparts.size() > 1)
			apiPort = addressparts[1].toInt();
		else
			apiPort   = API_DEFAULT_PORT;

		// TODO: Replace with valid identification code

		//		initRestAPI(apiHost, apiPort);

		//		QString resource = QString("%1/%2/%3").arg( API_LIGHTS ).arg( lightId ).arg( API_STATE);
		//		_restApi->setPath(resource);

		//		QString stateCmd;
		//		stateCmd += QString("\"%1\":%2,").arg( API_STATE_ON ).arg( API_STATE_VALUE_TRUE );
		//		stateCmd += QString("\"%1\":\"%2\"").arg( "alert" ).arg( "select" );
		//		stateCmd = "{" + stateCmd + "}";

		//		// Perform request
		//		httpResponse response = _restApi->put(stateCmd);
		//		if ( response.error() )
		//		{
		//			Warning (_log, "%s identification failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		//		}
	}
}

int LedDeviceWled::write(const std::vector<ColorRgb> &ledValues)
{
	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	return writeBytes((unsigned)_ledRGBCount, dataPtr);
}
