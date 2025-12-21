// Local-Hyperion includes
#include "LedDeviceHomeAssistant.h"

#include <algorithm>

#include <ssdp/SSDPDiscover.h>
#include <utils/NetUtils.h>
#include <utils/ColorRgb.h>

// Constants
namespace {
	// Configuration settings
	const char CONFIG_HOST[] = "host";
	const char CONFIG_PORT[] = "port";
	const char CONFIG_USE_SSL[] = "useSsl";
	const char CONFIG_AUTH_TOKEN[] = "token";
	const char CONFIG_ENITYIDS[] = "entityIds";
	const char CONFIG_BRIGHTNESS[] = "brightness";
	const char CONFIG_BRIGHTNESS_OVERWRITE[] = "overwriteBrightness";
	const char CONFIG_FULL_BRIGHTNESS_AT_START[] = "fullBrightnessAtStart";
	const char CONFIG_TRANSITIONTIME[] = "transitionTime";

	const bool DEFAULT_IS_BRIGHTNESS_OVERWRITE = true;
	const bool DEFAULT_IS_FULL_BRIGHTNESS_AT_START = true;
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
	const char TRANSITION[] = "transition";
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
	, _useSsl(false)
	, _isBrightnessOverwrite(DEFAULT_IS_BRIGHTNESS_OVERWRITE)
	, _isFullBrightnessAtStart(DEFAULT_IS_FULL_BRIGHTNESS_AT_START)
	, _brightness(BRI_MAX)
	, _transitionTime(0)
{
	NetUtils::discoverMdnsServices(_activeDeviceType);
}

LedDeviceHomeAssistant::~LedDeviceHomeAssistant()
{
}

LedDevice* LedDeviceHomeAssistant::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceHomeAssistant(deviceConfig);
}

bool LedDeviceHomeAssistant::init(const QJsonObject& deviceConfig)
{
	if (!LedDevice::init(deviceConfig))
	{
		return false;
	}

	bool isInitOK{ false };
	// Overwrite non supported/required features
	if (deviceConfig["rewriteTime"].toInt(0) > 0)
	{
		Info(_log, "Home Assistant lights do not require rewrites. Refresh time is ignored.");
		setRewriteTime(0);
	}

	//Set hostname as per configuration and default port
	_hostName = deviceConfig[CONFIG_HOST].toString();
	_apiPort = deviceConfig[CONFIG_PORT].toInt(API_DEFAULT_PORT);
	_useSsl = deviceConfig[CONFIG_USE_SSL].toBool(false);
	_bearerToken = deviceConfig[CONFIG_AUTH_TOKEN].toString();

	_isBrightnessOverwrite = _devConfig[CONFIG_BRIGHTNESS_OVERWRITE].toBool(DEFAULT_IS_BRIGHTNESS_OVERWRITE);
	_isFullBrightnessAtStart = _devConfig[CONFIG_FULL_BRIGHTNESS_AT_START].toBool(DEFAULT_IS_FULL_BRIGHTNESS_AT_START);
	_brightness = _devConfig[CONFIG_BRIGHTNESS].toInt(BRI_MAX);
	int transitionTimeMs = _devConfig[CONFIG_TRANSITIONTIME].toInt(0);
	_transitionTime = transitionTimeMs / 1000.0;

	Debug(_log, "Hostname/IP       : %s", QSTRING_CSTR(_hostName));
	Debug(_log, "Port              : %d", _apiPort);
	Debug(_log, "Use SSL           : %s", _useSsl ? "Yes" : "No");

	Debug(_log, "Overwrite Brightn.: %s", _isBrightnessOverwrite ? "Yes" : "No");
	Debug(_log, "Set Brightness to : %d", _brightness);
	Debug(_log, "Full Bri. at start: %s", _isFullBrightnessAtStart ? "Yes" : "No");
	Debug(_log, "Transition Time   : %d ms", transitionTimeMs);

	_lightEntityIds = _devConfig[CONFIG_ENITYIDS].toVariant().toStringList();
	auto configuredLightsCount = _lightEntityIds.size();

	if (configuredLightsCount == 0)
	{
		this->setInError("No light entity-ids configured");
		isInitOK = false;
	}
	else
	{
		Debug(_log, "Lights configured : %d", configuredLightsCount);
		isInitOK = true;
	}


	return isInitOK;
}

bool LedDeviceHomeAssistant::initLedsConfiguration()
{
	bool isInitOK = false;

	//Currently on one light is supported
	QString lightEntityId = _lightEntityIds[0];

	//Get properties for configured light entitiy to check availability
	_restApi->setPath({ API_STATES, lightEntityId });
	httpResponse response = _restApi->get();
	if (response.error())
	{
		QString errorReason = QString("%1 get properties failed with error: '%2'").arg(_activeDeviceType, response.getErrorReason());
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
	if (_hostName.isNull())
	{
		Error(_log, "Empty hostname or IP address. REST API cannot be initiatised.");
		return false;
	}

	if (_restApi == nullptr)
	{
		if (_apiPort == 0)
		{
			_apiPort = API_DEFAULT_PORT;
		}
		_restApi.reset(new ProviderRestApi(_useSsl ? "https" : "http", _hostName, _apiPort));

		_restApi->setLogger(_log);
		_restApi->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
		_restApi->setHeader("Authorization", QByteArrayLiteral("Bearer ") + _bearerToken.toUtf8());

		//Base-path is api-path
		_restApi->setBasePath(API_BASE_PATH);
	}
	return true;
}

int LedDeviceHomeAssistant::open()
{
	_isDeviceReady = false;
	this->setIsRecoverable(true);

	NetUtils::convertMdnsToIp(_log, _hostName, _apiPort);
	if (!openRestAPI())
	{
		return -1;
	}
	
	// Read LedDevice configuration and validate against device configuration
	if (initLedsConfiguration())
	{
		// Everything is OK, device is ready
		_isDeviceReady = true;
	}
	
	return _isDeviceReady ? 0 : -1;
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
	deviceList = NetUtils::getMdnsServicesDiscovered(_activeDeviceType);
#else
	QString discoveryMethod("ssdp");
	deviceList = discoverSsdp();
#endif

	devicesDiscovered.insert("discoveryMethod", discoveryMethod);
	devicesDiscovered.insert("devices", deviceList);

	return devicesDiscovered;
}

QJsonObject LedDeviceHomeAssistant::getProperties(const QJsonObject& params)
{
	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = params[CONFIG_PORT].toInt(API_DEFAULT_PORT);
	_useSsl = params[CONFIG_USE_SSL].toBool(false);
	_bearerToken = params[CONFIG_AUTH_TOKEN].toString("");

	Info(_log, "Get properties for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName));

	NetUtils::convertMdnsToIp(_log, _hostName, _apiPort);
	if (!openRestAPI())
	{
		return {};
	}

	QJsonObject properties;

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
		for (const QJsonValue& value : jsonArray)
		{
			QJsonObject obj = value.toObject();
			QString entityId = obj[ENTITY_ID].toString();

			if (entityId.startsWith("light."))
			{
				filteredVector.append(obj);
			}
		}

		// Sort the filtered vector by "friendly_name" in ascending order
		std::sort(filteredVector.begin(), filteredVector.end(), [](const QJsonValue& a, const QJsonValue& b) {
			QString nameA = a.toObject()["attributes"].toObject()["friendly_name"].toString();
			QString nameB = b.toObject()["attributes"].toObject()["friendly_name"].toString();
			return nameA < nameB;  // Ascending order
			});
		// Convert the sorted vector back to a QJsonArray
		QJsonArray sortedArray;
		for (const QJsonValue& value : filteredVector) {
			sortedArray.append(value);
		}

		propertiesDetails.insert("lightEntities", sortedArray);
	}

	if (!propertiesDetails.isEmpty())
	{
		propertiesDetails.insert("ledCount", 1);
	}
	properties.insert("properties", propertiesDetails);
	
	return properties;
}

void LedDeviceHomeAssistant::identify(const QJsonObject& params)
{
	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = params[CONFIG_PORT].toInt(API_DEFAULT_PORT);
	_useSsl = params[CONFIG_USE_SSL].toBool(false);
	_bearerToken = params[CONFIG_AUTH_TOKEN].toString("");

	Info(_log, "Identify %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName));

	NetUtils::convertMdnsToIp(_log, _hostName, _apiPort);
	if ( !openRestAPI() )
	{
		return;
	}

	QJsonArray lightEntityIds = params[ENTITY_ID].toArray();

	_restApi->setPath(API_LIGHT_TURN_ON);
	QJsonObject serviceAttributes{ {ENTITY_ID, lightEntityIds} };
	serviceAttributes.insert(FLASH, "short");

	httpResponse response = _restApi->post(serviceAttributes);
	if (response.error())
	{
		Warning(_log, "%s identification failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
	}
}

bool LedDeviceHomeAssistant::powerOn()
{
	if ( !_isDeviceReady)
	{
		return false;
	}

	bool isOn = false;

	_restApi->setPath(API_LIGHT_TURN_ON);
	QJsonObject serviceAttributes{ {ENTITY_ID, QJsonArray::fromStringList(_lightEntityIds)} };

	if (_isFullBrightnessAtStart)
	{
		serviceAttributes.insert(BRIGHTNESS, BRI_MAX);
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

	return isOn;
}

bool LedDeviceHomeAssistant::powerOff()
{
	if ( !_isDeviceReady)
	{
		return false;
	}

	bool isOff = true;
	_restApi->setPath(API_LIGHT_TURN_OFF);
	QJsonObject serviceAttributes{ {ENTITY_ID, QJsonArray::fromStringList(_lightEntityIds)} };
	httpResponse response = _restApi->post(serviceAttributes);
	if (response.error())
	{
		QString errorReason = QString("Power-off request failed with error: '%1'").arg(response.getErrorReason());
		this->setInError(errorReason);
		isOff = false;
	}
	return isOff;
}

int LedDeviceHomeAssistant::write(const QVector<ColorRgb>& ledValues)
{
	QJsonObject serviceAttributes{ {ENTITY_ID, QJsonArray::fromStringList(_lightEntityIds)} };
	ColorRgb ledValue = ledValues.at(0);

	//    http://hostname:port/api/services/light/turn_on
	//    {
	//      "entity_id": [ entity-IDs ],
	//      "rgb_color": [R,G,B]
	//    }

	_restApi->setPath(API_LIGHT_TURN_ON);
	serviceAttributes.insert(RGB_COLOR, QJsonArray{ ledValue.red, ledValue.green, ledValue.blue });

	int brightness;
	if (!_isBrightnessOverwrite)
	{
		brightness = qBound(0, qRound(0.2126 * ledValue.red + 0.7152 * ledValue.green + 0.0722 * ledValue.blue), 255);
	}
	else
	{
		brightness = _brightness;
	}

	serviceAttributes.insert(BRIGHTNESS, brightness);

	if (_transitionTime > 0)
	{
		serviceAttributes.insert(TRANSITION, _transitionTime);
	}

	httpResponse response = _restApi->post(serviceAttributes);
	if (response.error())
	{
		Warning(_log, "Updating lights failed with error: '%s'", QSTRING_CSTR(response.getErrorReason()));
		return -1;
	}

	return 0;
}
