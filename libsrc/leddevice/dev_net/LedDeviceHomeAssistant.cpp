// Local-Hyperion includes
#include "LedDeviceHomeAssistant.h"

#include <ssdp/SSDPDiscover.h>
// mDNS discover
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#endif
#include <utils/NetUtils.h>

#include <algorithm>

// Constants
namespace {
const bool verbose = false;

// Configuration settings
const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";
const char CONFIG_AUTH_TOKEN[] = "token";
const char CONFIG_ENITYIDS[] = "entityIds";
const char CONFIG_BRIGHTNESS[] = "brightness";
const char CONFIG_BRIGHTNESS_OVERWRITE[] = "overwriteBrightness";

const bool DEFAULT_IS_BRIGHTNESS_OVERWRITE = true;
const int  BRI_MAX = 255;

// Home Assistant API
const int  API_DEFAULT_PORT = 8123;
const char API_BASE_PATH[] = "/api/";
const char API_STATES[] = "states";
const char API_LIGHT_TURN_ON[] = "services/light/turn_on";
const char API_LIGHT_TURN_OFF[] = "services/light/turn_off";

const char ENTITY_ID[] = "entity_id";
const char RGB_COLOR[] = "rgb_color";
const char BRIGHTNESS[] = "brightness";
const char FLASH[] = "flash";

// // Home Assistant ssdp services
const char SSDP_ID[] = "ssdp:all";
const char SSDP_FILTER_HEADER[] = "ST";
const char SSDP_FILTER[] = "(.*)home-assistant.io(.*)";

} //End of constants

LedDeviceHomeAssistant::LedDeviceHomeAssistant(const QJsonObject& deviceConfig)
	: LedDevice(deviceConfig)
	, _restApi(nullptr)
	, _apiPort(API_DEFAULT_PORT)
	, _isBrightnessOverwrite(DEFAULT_IS_BRIGHTNESS_OVERWRITE)
	, _brightness (BRI_MAX)
{
#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(MdnsBrowser::getInstance().data(), "browseForServiceType",
							  Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType(_activeDeviceType)));
#endif
}

LedDevice* LedDeviceHomeAssistant::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceHomeAssistant(deviceConfig);
}

LedDeviceHomeAssistant::~LedDeviceHomeAssistant()
{
	delete _restApi;
	_restApi = nullptr;
}

bool LedDeviceHomeAssistant::init(const QJsonObject& deviceConfig)
{
	bool isInitOK{ false };

	if ( LedDevice::init(deviceConfig) )
	{
		// Overwrite non supported/required features
		if (deviceConfig["rewriteTime"].toInt(0) > 0)
		{
			Info(_log, "Home Assistant lights do not require rewrites. Refresh time is ignored.");
			setRewriteTime(0);
		}
		DebugIf(verbose, _log, "deviceConfig: [%s]", QString(QJsonDocument(_devConfig).toJson(QJsonDocument::Compact)).toUtf8().constData());

		//Set hostname as per configuration and default port
		_hostName = deviceConfig[CONFIG_HOST].toString();
		_apiPort = deviceConfig[CONFIG_PORT].toInt(API_DEFAULT_PORT);
		_bearerToken = deviceConfig[CONFIG_AUTH_TOKEN].toString();

		_isBrightnessOverwrite = _devConfig[CONFIG_BRIGHTNESS_OVERWRITE].toBool(DEFAULT_IS_BRIGHTNESS_OVERWRITE);
		_brightness = _devConfig[CONFIG_BRIGHTNESS].toInt(BRI_MAX);

		Debug(_log, "Hostname/IP       : %s", QSTRING_CSTR(_hostName));
		Debug(_log, "Port              : %d", _apiPort );

		Debug(_log, "Overwrite Brightn.: %d", _isBrightnessOverwrite);
		Debug(_log, "Set Brightness to : %d", _brightness);

		_lightEntityIds = _devConfig[ CONFIG_ENITYIDS ].toVariant().toStringList();
		int configuredLightsCount = _lightEntityIds.size();

		if ( configuredLightsCount == 0 )
		{
			this->setInError( "No light entity-ids configured" );
			isInitOK = false;
		}
		else
		{
			Debug(_log, "Lights configured : %d", configuredLightsCount );
			isInitOK = true;
		}
	}

	return isInitOK;
}

bool LedDeviceHomeAssistant::initLedsConfiguration()
{
	bool isInitOK = false;

	//Currently on one light is supported
	QString lightEntityId = _lightEntityIds[0];

	//Get properties for configured light entitiy to check availability
	_restApi->setPath({ API_STATES, lightEntityId});
	httpResponse response = _restApi->get();
	if (response.error())
	{
		QString errorReason = QString("%1 get properties failed with error: '%2'").arg(_activeDeviceType,response.getErrorReason());
		this->setInError(errorReason);
	}
	else
	{
		QJsonObject propertiesDetails = response.getBody().object();
		if (propertiesDetails.isEmpty())
		{
			QString errorReason = QString("Light [%1] does not exist").arg(lightEntityId);
			this->setInError(errorReason);
		}
		else
		{
			if (propertiesDetails.value("state").toString().compare("unavailable") == 0)
			{
				Warning(_log, "Light [%s] is currently unavailable", QSTRING_CSTR(lightEntityId));
			}
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDeviceHomeAssistant::openRestAPI()
{
	bool isInitOK{ true };

	if (_restApi == nullptr)
	{
		if (_apiPort == 0)
		{
			_apiPort = API_DEFAULT_PORT;
		}

		_restApi = new ProviderRestApi(_address.toString(), _apiPort);
		_restApi->setLogger(_log);

		_restApi->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
		_restApi->setHeader("Authorization", QByteArrayLiteral("Bearer ") + _bearerToken.toUtf8());

		//Base-path is api-path
		_restApi->setBasePath(API_BASE_PATH);
	}
	return isInitOK;
}

int LedDeviceHomeAssistant::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if (openRestAPI())
		{
			// Read LedDevice configuration and validate against device configuration
			if (initLedsConfiguration())
			{
				// Everything is OK, device is ready
				_isDeviceReady = true;
				retval = 0;
			}
		}
		else
		{
			_restApi->setHost(_address.toString());
			_restApi->setPort(_apiPort);
		}
	}
	return retval;
}

QJsonArray LedDeviceHomeAssistant::discoverSsdp() const
{
	QJsonArray deviceList;
	SSDPDiscover ssdpDiscover;
	ssdpDiscover.skipDuplicateKeys(true);
	ssdpDiscover.setSearchFilter(SSDP_FILTER, SSDP_FILTER_HEADER);
	QString searchTarget = SSDP_ID;

	if (ssdpDiscover.discoverServices(searchTarget) > 0)
	{
		deviceList = ssdpDiscover.getServicesDiscoveredJson();
	}
	return deviceList;
}

QJsonObject LedDeviceHomeAssistant::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType);

	QJsonArray deviceList;

#ifdef ENABLE_MDNS
	QString discoveryMethod("mDNS");
	deviceList = MdnsBrowser::getInstance().data()->getServicesDiscoveredJson(
					 MdnsServiceRegister::getServiceType(_activeDeviceType),
					 MdnsServiceRegister::getServiceNameFilter(_activeDeviceType),
					 DEFAULT_DISCOVER_TIMEOUT
					 );
#else
	QString discoveryMethod("ssdp");
	deviceList = discoverSsdp();
#endif

	devicesDiscovered.insert("discoveryMethod", discoveryMethod);
	devicesDiscovered.insert("devices", deviceList);

	DebugIf(verbose, _log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}

QJsonObject LedDeviceHomeAssistant::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());
	QJsonObject properties;

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;
	_bearerToken = params[CONFIG_AUTH_TOKEN].toString("");

	Info(_log, "Get properties for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName));

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if (openRestAPI())
		{
			QString filter = params["filter"].toString("");
			_restApi->setPath(filter);

			// Perform request
			httpResponse response = _restApi->get();
			if (response.error())
			{
				Warning(_log, "%s get properties failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			}

			QJsonObject propertiesDetails;
			const QJsonDocument jsonDoc = response.getBody();
			if (jsonDoc.isArray()) {
				const QJsonArray jsonArray = jsonDoc.array();
				QVector<QJsonValue> filteredVector;

				// Iterate over the array and filter objects with entity_id starting with "light."
				for (const QJsonValue &value : jsonArray)
				{
					QJsonObject obj = value.toObject();
					QString entityId = obj[ENTITY_ID].toString();

					if (entityId.startsWith("light."))
					{
						filteredVector.append(obj);
					}
				}

				// Sort the filtered vector by "friendly_name" in ascending order
				std::sort(filteredVector.begin(), filteredVector.end(), [](const QJsonValue &a, const QJsonValue &b) {
					QString nameA = a.toObject()["attributes"].toObject()["friendly_name"].toString();
					QString nameB = b.toObject()["attributes"].toObject()["friendly_name"].toString();
					return nameA < nameB;  // Ascending order
				});
				// Convert the sorted vector back to a QJsonArray
				QJsonArray sortedArray;
				for (const QJsonValue &value : filteredVector) {
					sortedArray.append(value);
				}

				propertiesDetails.insert("lightEntities", sortedArray);

			}

			if (!propertiesDetails.isEmpty())
			{
				propertiesDetails.insert("ledCount", 1);
			}
			properties.insert("properties", propertiesDetails);
		}

		DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData());
	}
	return properties;
}

void LedDeviceHomeAssistant::identify(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;
	_bearerToken = params[CONFIG_AUTH_TOKEN].toString("");

	Info(_log, "Identify %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName));

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if (openRestAPI())
		{
			QJsonArray lightEntityIds = params[ ENTITY_ID ].toArray();

			_restApi->setPath(API_LIGHT_TURN_ON);
			QJsonObject serviceAttributes{{ENTITY_ID, lightEntityIds}};
			serviceAttributes.insert(FLASH, "short");

			httpResponse response = _restApi->post(serviceAttributes);
			if (response.error())
			{
				Warning(_log, "%s identification failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			}
		}
	}
}

bool LedDeviceHomeAssistant::powerOn()
{
	bool isOn = false;
	if (_isDeviceReady)
	{
		_restApi->setPath(API_LIGHT_TURN_ON);
		QJsonObject serviceAttributes {{ENTITY_ID, QJsonArray::fromStringList(_lightEntityIds)}};

		if (_isBrightnessOverwrite)
		{
			serviceAttributes.insert(BRIGHTNESS, _brightness);
		}

		httpResponse response = _restApi->post(serviceAttributes);
		if (response.error())
		{
			QString errorReason = QString("Power-on request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError(errorReason);
			isOn = false;
		}
		else {
			isOn = true;
		}
	}
	return isOn;
}

bool LedDeviceHomeAssistant::powerOff()
{
	bool isOff = true;
	if (_isDeviceReady)
	{
		_restApi->setPath(API_LIGHT_TURN_OFF);
		QJsonObject serviceAttributes {{ENTITY_ID, QJsonArray::fromStringList(_lightEntityIds)}};
		httpResponse response = _restApi->post(serviceAttributes);
		if (response.error())
		{
			QString errorReason = QString("Power-off request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError(errorReason);
			isOff = false;
		}
	}
	return isOff;
}

int LedDeviceHomeAssistant::write(const std::vector<ColorRgb>& ledValues)
{
	int retVal = 0;

	//    http://hostname:port/api/services/light/turn_on
	//    {
	//      "entity_id": [ entity-IDs ],
	//      "rgb_color": [R,G,B]
	//    }

	_restApi->setPath(API_LIGHT_TURN_ON);
	QJsonObject serviceAttributes {{ENTITY_ID, QJsonArray::fromStringList(_lightEntityIds)}};

	ColorRgb ledValue = ledValues.at(0);
	QJsonArray rgbColor {ledValue.red, ledValue.green, ledValue.blue};
	serviceAttributes.insert(RGB_COLOR, rgbColor);
	httpResponse response = _restApi->post(serviceAttributes);
	if (response.error())
	{
		Warning(_log,"Updating lights failed with error: '%s'", QSTRING_CSTR(response.getErrorReason()) );
		retVal = -1;
	}

	return retVal;
}
