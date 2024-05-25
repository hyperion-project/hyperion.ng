// Local-Hyperion includes
#include "LedDeviceNanoleaf.h"

//std includes
#include <sstream>
#include <iomanip>
#include <cmath>

// Qt includes
#include <QNetworkReply>
#include <QtEndian>

#include <ssdp/SSDPDiscover.h>
#include <utils/QStringUtils.h>

// mDNS discover
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#endif
#include <utils/NetUtils.h>

// Constants
namespace {
	const bool verbose = false;
	const bool verbose3 = false;

	// Configuration settings
	const char CONFIG_HOST[] = "host";
	const char CONFIG_AUTH_TOKEN[] = "token";
	const char CONFIG_RESTORE_STATE[] = "restoreOriginalState";
	const char CONFIG_BRIGHTNESS[] = "brightness";
	const char CONFIG_BRIGHTNESS_OVERWRITE[] = "overwriteBrightness";

	const char CONFIG_PANEL_ORDER_TOP_DOWN[] = "panelOrderTopDown";
	const char CONFIG_PANEL_ORDER_LEFT_RIGHT[] = "panelOrderLeftRight";

	const bool DEFAULT_IS_RESTORE_STATE = true;
	const bool DEFAULT_IS_BRIGHTNESS_OVERWRITE = true;
	const int BRI_MAX = 100;

	// Panel configuration settings
	const char PANEL_GLOBALORIENTATION[] = "globalOrientation";
	const char PANEL_GLOBALORIENTATION_VALUE[] = "value";
	const char PANEL_LAYOUT[] = "layout";
	const char PANEL_NUM[] = "numPanels";
	const char PANEL_ID[] = "panelId";
	const char PANEL_POSITIONDATA[] = "positionData";
	const char PANEL_SHAPE_TYPE[] = "shapeType";
	const char PANEL_POS_X[] = "x";
	const char PANEL_POS_Y[] = "y";

	// List of State Information
	const char STATE_ON[] = "on";
	const char STATE_BRI[] = "brightness";
	const char STATE_HUE[] = "hue";
	const char STATE_SAT[] = "sat";
	const char STATE_CT[] = "ct";
	const char STATE_COLORMODE[] = "colorMode";
	const QStringList COLOR_MODES{ "hs", "ct", "effect" };
	const char STATE_VALUE[] = "value";

	// Device Data elements
	const char DEV_DATA_NAME[] = "name";
	const char DEV_DATA_MODEL[] = "model";
	const char DEV_DATA_MANUFACTURER[] = "manufacturer";
	const char DEV_DATA_FIRMWAREVERSION[] = "firmwareVersion";

	// Nanoleaf Stream Control elements
	const quint16 STREAM_CONTROL_DEFAULT_PORT = 60222;

	// Nanoleaf OpenAPI URLs
	const int API_DEFAULT_PORT = 16021;
	const char API_BASE_PATH[] = "/api/v1/%1/";
	const char API_ROOT[] = "";
	const char API_EXT_MODE_STRING_V2[] = "{\"write\" : {\"command\" : \"display\", \"animType\" : \"extControl\", \"extControlVersion\" : \"v2\"}}";
	const char API_STATE[] = "state";
	const char API_PANELLAYOUT[] = "panelLayout";
	const char API_EFFECT[] = "effects";
	const char API_IDENTIFY[] = "identify";
	const char API_ADD_USER[] = "new";
	const char API_EFFECT_SELECT[] = "select";

	//Nanoleaf Control data stream
	const int STREAM_FRAME_PANEL_NUM_SIZE = 2;
	const int STREAM_FRAME_PANEL_INFO_SIZE = 8;

	// Nanoleaf ssdp services
	const char SSDP_ID[] = "ssdp:all";
	const char SSDP_FILTER_HEADER[] = "ST";
	const char SSDP_NANOLEAF[] = "nanoleaf:nl*";
	const char SSDP_LIGHTPANELS[] = "nanoleaf_aurora:light";

	const double ROTATION_STEPS_DEGREE = 15.0;

} //End of constants

// Nanoleaf external control versions
enum EXTCONTROLVERSIONS {
	EXTCTRLVER_V1 = 1,
	EXTCTRLVER_V2
};

LedDeviceNanoleaf::LedDeviceNanoleaf(const QJsonObject& deviceConfig)
	: ProviderUdp(deviceConfig)
	, _restApi(nullptr)
	, _apiPort(API_DEFAULT_PORT)
	, _topDown(true)
	, _leftRight(true)
	, _extControlVersion(EXTCTRLVER_V2)
	, _panelLedCount(0)
{
#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(MdnsBrowser::getInstance().data(), "browseForServiceType",
		Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType(_activeDeviceType)));
#endif
}

LedDevice* LedDeviceNanoleaf::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceNanoleaf(deviceConfig);
}

LedDeviceNanoleaf::~LedDeviceNanoleaf()
{
	delete _restApi;
	_restApi = nullptr;
}

bool LedDeviceNanoleaf::init(const QJsonObject& deviceConfig)
{
	bool isInitOK{ false };

	// Overwrite non supported/required features
	setLatchTime(0);
	setRewriteTime(0);

	if (deviceConfig["rewriteTime"].toInt(0) > 0)
	{
		Info(_log, "Device Nanoleaf does not require rewrites. Refresh time is ignored.");
	}

	DebugIf(verbose, _log, "deviceConfig: [%s]", QString(QJsonDocument(_devConfig).toJson(QJsonDocument::Compact)).toUtf8().constData());

	if (ProviderUdp::init(deviceConfig))
	{
		//Set hostname as per configuration and default port
		_hostName = deviceConfig[CONFIG_HOST].toString();
		_port = STREAM_CONTROL_DEFAULT_PORT;
		_apiPort = API_DEFAULT_PORT;
		_authToken = deviceConfig[CONFIG_AUTH_TOKEN].toString();

		_isRestoreOrigState = _devConfig[CONFIG_RESTORE_STATE].toBool(DEFAULT_IS_RESTORE_STATE);
		_isBrightnessOverwrite = _devConfig[CONFIG_BRIGHTNESS_OVERWRITE].toBool(DEFAULT_IS_BRIGHTNESS_OVERWRITE);
		_brightness = _devConfig[CONFIG_BRIGHTNESS].toInt(BRI_MAX);

		Debug(_log, "Hostname/IP       : %s", QSTRING_CSTR(_hostName));
		Debug(_log, "RestoreOrigState  : %d", _isRestoreOrigState);
		Debug(_log, "Overwrite Brightn.: %d", _isBrightnessOverwrite);
		Debug(_log, "Set Brightness to : %d", _brightness);

		// Read panel organisation configuration
		_topDown = deviceConfig[CONFIG_PANEL_ORDER_TOP_DOWN].toString("top2down") == "top2down";
		_leftRight = deviceConfig[CONFIG_PANEL_ORDER_LEFT_RIGHT].toString("left2right") == "left2right";

		isInitOK = true;
	}
	return isInitOK;
}

int LedDeviceNanoleaf::getHwLedCount(const QJsonObject& jsonLayout) const
{
	int hwLedCount{ 0 };

	const QJsonArray positionData = jsonLayout[PANEL_POSITIONDATA].toArray();
	for (const QJsonValue& value : positionData)
	{
		QJsonObject panelObj = value.toObject();
		int panelId = panelObj[PANEL_ID].toInt();
		int panelshapeType = panelObj[PANEL_SHAPE_TYPE].toInt();

		DebugIf(verbose, _log, "Panel [%d] - Type: [%d]", panelId, panelshapeType);

		if (hasLEDs(static_cast<SHAPETYPES>(panelshapeType)))
		{
			++hwLedCount;
		}
		else
		{
			DebugIf(verbose, _log, "Rhythm/Shape/Lines Controller panel skipped.");
		}
	}
	return hwLedCount;
}

bool LedDeviceNanoleaf::hasLEDs(const SHAPETYPES& panelshapeType) const
{
	bool hasLED {true};
	// Skip non LED panel types
	switch (panelshapeType)
	{
	case SHAPES_CONTROLLER:
	case LINES_CONECTOR:
	case CONTROLLER_CAP:
	case POWER_CONNECTOR:
	case RHYTM:
		DebugIf(verbose, _log, "Rhythm/Shape/Lines Controller panel skipped.");
		hasLED = false;
		break;
	default:
		break;
	}

	return hasLED;
}

bool LedDeviceNanoleaf::initLedsConfiguration()
{
	bool isInitOK = true;

	//Get Nanoleaf device details and configuration

	// Read Panel count and panel Ids
	_restApi->setPath(API_ROOT);
	httpResponse response = _restApi->get();
	if (response.error())
	{
		QString errorReason = QString("Getting device details failed with error: '%1'").arg(response.getErrorReason());
		this->setInError(errorReason);
		isInitOK = false;
	}
	else
	{
		QJsonObject jsonAllPanelInfo = response.getBody().object();

		QString deviceName = jsonAllPanelInfo[DEV_DATA_NAME].toString();
		_deviceModel = jsonAllPanelInfo[DEV_DATA_MODEL].toString();
		QString deviceManufacturer = jsonAllPanelInfo[DEV_DATA_MANUFACTURER].toString();
		_deviceFirmwareVersion = jsonAllPanelInfo[DEV_DATA_FIRMWAREVERSION].toString();

		Debug(_log, "Name           : %s", QSTRING_CSTR(deviceName));
		Debug(_log, "Model          : %s", QSTRING_CSTR(_deviceModel));
		Debug(_log, "Manufacturer   : %s", QSTRING_CSTR(deviceManufacturer));
		Debug(_log, "FirmwareVersion: %s", QSTRING_CSTR(_deviceFirmwareVersion));

		// Get panel details from /panelLayout/layout
		QJsonObject jsonPanelLayout = jsonAllPanelInfo[API_PANELLAYOUT].toObject();

		const QJsonObject globalOrientation = jsonPanelLayout[PANEL_GLOBALORIENTATION].toObject();
		int orientation = globalOrientation[PANEL_GLOBALORIENTATION_VALUE].toInt();

		int degreesToRotate {orientation};
		bool isRotated {false};
		if (degreesToRotate > 0)
		{
			isRotated = true;
			int degreeRounded = static_cast<int>(round(degreesToRotate / ROTATION_STEPS_DEGREE) * ROTATION_STEPS_DEGREE);
			degreesToRotate = (degreeRounded +360) % 360;
		}

		//Nanoleaf orientation is counter-clockwise
		degreesToRotate *= -1;

		double radians = (degreesToRotate * std::acos(-1)) / 180;
		DebugIf(verbose, _log, "globalOrientation: %d, degreesToRotate: %d, radians: %0.2f", orientation, degreesToRotate, radians);

		QJsonObject jsonLayout = jsonPanelLayout[PANEL_LAYOUT].toObject();

		_panelLedCount = getHwLedCount(jsonLayout);
		_devConfig["hardwareLedCount"] = _panelLedCount;

		int panelNum = jsonLayout[PANEL_NUM].toInt();
		const QJsonArray positionData = jsonLayout[PANEL_POSITIONDATA].toArray();

		std::map<int, std::map<int, int>> panelMap;

		// Loop over all children.
		for (const QJsonValue& value : positionData)
		{
			QJsonObject panelObj = value.toObject();

			int panelId = panelObj[PANEL_ID].toInt();
			int panelshapeType = panelObj[PANEL_SHAPE_TYPE].toInt();
			int posX = panelObj[PANEL_POS_X].toInt();
			int posY = panelObj[PANEL_POS_Y].toInt();

			int panelX;
			int panelY;
			if (isRotated)
			{
				panelX = static_cast<int>(round(posX * cos(radians) - posY * sin(radians)));
				panelY = static_cast<int>(round(posX * sin(radians) + posY * cos(radians)));
			}
			else
			{
				panelX = posX;
				panelY = posY;
			}

			if (hasLEDs(static_cast<SHAPETYPES>(panelshapeType)))
			{
				panelMap[panelY][panelX] = panelId;
				DebugIf(verbose, _log, "Use  Panel [%d] (%d,%d) - Type: [%d]", panelId, panelX, panelY, panelshapeType);
			}
			else
			{
				DebugIf(verbose, _log, "Skip Panel [%d] (%d,%d) - Type: [%d]", panelId, panelX, panelY, panelshapeType);
			}
		}

		// Travers panels top down
		_panelIds.clear();
		for (auto posY = panelMap.crbegin(); posY != panelMap.crend(); ++posY)
		{
			// Sort panels left to right
			if (_leftRight)
			{
				for (auto posX = posY->second.cbegin(); posX != posY->second.cend(); ++posX)
				{
					DebugIf(verbose, _log, "panelMap[%d][%d]=%d", posY->first, posX->first, posX->second);

					if (_topDown)
					{
						_panelIds.push_back(posX->second);
					}
					else
					{
						_panelIds.push_front(posX->second);
					}
				}
			}
			else
			{
				// Sort panels right to left
				for (auto posX = posY->second.crbegin(); posX != posY->second.crend(); ++posX)
				{
					DebugIf(verbose, _log, "panelMap[%d][%d]=%d", posY->first, posX->first, posX->second);

					if (_topDown)
					{
						_panelIds.push_back(posX->second);
					}
					else
					{
						_panelIds.push_front(posX->second);
					}
				}
			}
		}

		Debug(_log, "PanelsNum      : %d", panelNum);
		Debug(_log, "PanelLedCount  : %d", _panelLedCount);
		Debug(_log, "Sort Top>Down  : %d", _topDown);
		Debug(_log, "Sort Left>Right: %d", _leftRight);

		DebugIf(verbose, _log, "PanelMap size  : %d", panelMap.size());
		DebugIf(verbose, _log, "PanelIds count : %d", _panelIds.size());

		// Check. if enough panels were found.
		int configuredLedCount = this->getLedCount();
		if (_panelLedCount < configuredLedCount)
		{
			QString errorReason = QString("Not enough panels [%1] for configured LEDs [%2] found!")
				.arg(_panelLedCount)
				.arg(configuredLedCount);
			this->setInError(errorReason, false);
			isInitOK = false;
		}
		else
		{
			if (_panelLedCount > this->getLedCount())
			{
				Info(_log, "%s: More panels [%d] than configured LEDs [%d].", QSTRING_CSTR(this->getActiveDeviceType()), _panelLedCount, configuredLedCount);
			}

			//Check that panel count matches working list created for processing
			if (_panelLedCount != _panelIds.size())
			{
				QString errorReason = QString("Number of available panels [%1] do not match panel-ID look-up list [%2]!")
					.arg(_panelLedCount)
					.arg(_panelIds.size());
				this->setInError(errorReason, false);
				isInitOK = false;
			}

		}
	}
	return isInitOK;
}

bool LedDeviceNanoleaf::openRestAPI()
{
	bool isInitOK{ true };

	if (_restApi == nullptr)
	{
		_restApi = new ProviderRestApi(_address.toString(), _apiPort);
		_restApi->setLogger(_log);

		//Base-path is api-path + authentication token
		_restApi->setBasePath(QString(API_BASE_PATH).arg(_authToken));
	}
	return isInitOK;
}

int LedDeviceNanoleaf::open()
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
				if (ProviderUdp::open() == 0)
				{
					// Everything is OK, device is ready
					_isDeviceReady = true;
					retval = 0;
				}
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

QJsonArray LedDeviceNanoleaf::discover()
{
	QJsonArray deviceList;

	SSDPDiscover discover;

	// Search for Canvas and Light-Panels
	QString searchTargetFilter = QString("%1|%2").arg(SSDP_NANOLEAF, SSDP_LIGHTPANELS);

	discover.setSearchFilter(searchTargetFilter, SSDP_FILTER_HEADER);
	QString searchTarget = SSDP_ID;

	if (discover.discoverServices(searchTarget) > 0)
	{
		deviceList = discover.getServicesDiscoveredJson();
	}

	return deviceList;
}

QJsonObject LedDeviceNanoleaf::discover(const QJsonObject& /*params*/)
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
	deviceList = discover();
#endif

	devicesDiscovered.insert("discoveryMethod", discoveryMethod);
	devicesDiscovered.insert("devices", deviceList);

	DebugIf(verbose, _log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}

QJsonObject LedDeviceNanoleaf::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());
	QJsonObject properties;

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;
	_authToken = params[CONFIG_AUTH_TOKEN].toString("");

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
			QJsonObject propertiesDetails = response.getBody().object();
			if (!propertiesDetails.isEmpty())
			{
				QJsonObject jsonLayout = propertiesDetails.value(API_PANELLAYOUT).toObject().value(PANEL_LAYOUT).toObject();
				_panelLedCount = getHwLedCount(jsonLayout);
				propertiesDetails.insert("ledCount", getHwLedCount(jsonLayout));
			}
			properties.insert("properties", propertiesDetails);
		}

		DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData());
	}
	return properties;
}

void LedDeviceNanoleaf::identify(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;
	_authToken = params[CONFIG_AUTH_TOKEN].toString("");

	Info(_log, "Identify %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName));

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if (openRestAPI())
		{
			_restApi->setPath(API_IDENTIFY);
			httpResponse response = _restApi->put();
			if (response.error())
			{
				Warning(_log, "%s identification failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			}
		}
	}
}

QJsonObject LedDeviceNanoleaf::addAuthorization(const QJsonObject& params)
{
	Debug(_log, "params: [%s]", QJsonDocument(params).toJson(QJsonDocument::Compact).constData());
	QJsonObject responseBody;

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;

	Info(_log, "Generate user authorization token for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName));

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if (openRestAPI())
		{
			_restApi->setBasePath(QString(API_BASE_PATH).arg(API_ADD_USER));
			httpResponse response = _restApi->post();
			if (response.error())
			{
				Warning(_log, "%s generating user authorization token failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			}
			else
			{
				Debug(_log, "Generated user authorization token: \"%s\"", QSTRING_CSTR(response.getBody().object().value("auth_token").toString()));
				responseBody = response.getBody().object();
			}
		}
	}
	return responseBody;
}

bool LedDeviceNanoleaf::powerOn()
{
	bool on = false;
	if (_isDeviceReady)
	{
		if (changeToExternalControlMode())
		{
			QJsonObject newState;

			QJsonObject onValue{ {STATE_VALUE, true} };
			newState.insert(STATE_ON, onValue);

			if (_isBrightnessOverwrite)
			{
				QJsonObject briValue{ {STATE_VALUE, _brightness} };
				newState.insert(STATE_BRI, briValue);
			}

			//Power-on Nanoleaf device
			_restApi->setPath(API_STATE);
			httpResponse response = _restApi->put(newState);
			if (response.error())
			{
				QString errorReason = QString("Power-on request failed with error: '%1'").arg(response.getErrorReason());
				this->setInError(errorReason);
				on = false;
			}
			else {
				on = true;
			}

		}
	}
	return on;
}

bool LedDeviceNanoleaf::powerOff()
{
	bool off = true;
	if (_isDeviceReady)
	{
		QJsonObject newState;

		QJsonObject onValue{ {STATE_VALUE, false} };
		newState.insert(STATE_ON, onValue);

		//Power-off the Nanoleaf device physically
		_restApi->setPath(API_STATE);
		httpResponse response = _restApi->put(newState);
		if (response.error())
		{
			QString errorReason = QString("Power-off request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError(errorReason);
			off = false;
		}
	}
	return off;
}

bool LedDeviceNanoleaf::storeState()
{
	bool rc = true;

	if (_isRestoreOrigState)
	{
		_restApi->setPath(API_STATE);

		httpResponse response = _restApi->get();
		if (response.error())
		{
			QString errorReason = QString("Storing device state failed with error: '%1'").arg(response.getErrorReason());
			setInError(errorReason);
			rc = false;
		}
		else
		{
			_originalStateProperties = response.getBody().object();
			DebugIf(verbose, _log, "state: [%s]", QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData());

			QJsonObject isOn = _originalStateProperties.value(STATE_ON).toObject();
			if (!isOn.isEmpty())
			{
				_originalIsOn = isOn[STATE_VALUE].toBool();
			}

			QJsonObject bri = _originalStateProperties.value(STATE_BRI).toObject();
			if (!bri.isEmpty())
			{
				_originalBri = bri[STATE_VALUE].toInt();
			}

			_originalColorMode = _originalStateProperties[STATE_COLORMODE].toString();

			switch (COLOR_MODES.indexOf(_originalColorMode)) {
			case 0:
			{
				// hs
				QJsonObject hue = _originalStateProperties.value(STATE_HUE).toObject();
				if (!hue.isEmpty())
				{
					_originalHue = hue[STATE_VALUE].toInt();
				}
				QJsonObject sat = _originalStateProperties.value(STATE_SAT).toObject();
				if (!sat.isEmpty())
				{
					_originalSat = sat[STATE_VALUE].toInt();
				}
				break;
			}
			case 1:
			{
				// ct
				QJsonObject ct = _originalStateProperties.value(STATE_CT).toObject();
				if (!ct.isEmpty())
				{
					_originalCt = ct[STATE_VALUE].toInt();
				}
				break;
			}
			case 2:
			{
				// effect
				_restApi->setPath(API_EFFECT);

				httpResponse responseEffects = _restApi->get();
				if (responseEffects.error())
				{
					QString errorReason = QString("Storing device state failed with error: '%1'").arg(responseEffects.getErrorReason());
					setInError(errorReason);
					rc = false;
				}
				else
				{
					QJsonObject effects = responseEffects.getBody().object();
					DebugIf(verbose, _log, "effects: [%s]", QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData());
					_originalEffect = effects[API_EFFECT_SELECT].toString();
                    _originalIsDynEffect = _originalEffect != "*Dynamic*" || _originalEffect == "*Solid*" || _originalEffect == "*ExtControl*";
				}
				break;
			}
			default:
				QString errorReason = QString("Unknown ColorMode: '%1'").arg(_originalColorMode);
				setInError(errorReason);
				rc = false;
				break;
			}
		}
	}
	return rc;
}

bool LedDeviceNanoleaf::restoreState()
{
	bool rc = true;

	if (_isRestoreOrigState)
	{
		QJsonObject newState;
		switch (COLOR_MODES.indexOf(_originalColorMode)) {
		case 0:
		{	// hs
			QJsonObject hueValue{ {STATE_VALUE, _originalHue} };
			newState.insert(STATE_HUE, hueValue);
			QJsonObject satValue{ {STATE_VALUE, _originalSat} };
			newState.insert(STATE_SAT, satValue);
			break;
		}
		case 1:
		{	// ct
			QJsonObject ctValue{ {STATE_VALUE, _originalCt} };
			newState.insert(STATE_CT, ctValue);
			break;
		}
		case 2:
		{	// effect
			if (!_originalIsDynEffect)
			{
				QJsonObject newEffect;
				newEffect[API_EFFECT_SELECT] = _originalEffect;
				_restApi->setPath(API_EFFECT);
				httpResponse response = _restApi->put(newEffect);
				if (response.error())
				{
					Warning(_log, "%s restoring effect failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
				}
			}
			else {
                Info(_log, "%s cannot restore dynamic or solid effects. Device is switched off instead", QSTRING_CSTR(_activeDeviceType));
				_originalIsOn = false;
			}
			break;
		}
		default:
			Warning(_log, "%s restoring failed with error: Unknown ColorMode", QSTRING_CSTR(_activeDeviceType));
			rc = false;
		}

		if (!_originalIsDynEffect)
		{
			QJsonObject briValue{ {STATE_VALUE, _originalBri} };
			newState.insert(STATE_BRI, briValue);
		}

		QJsonObject onValue{ {STATE_VALUE, _originalIsOn} };
		newState.insert(STATE_ON, onValue);

		_restApi->setPath(API_STATE);

		httpResponse response = _restApi->put(newState);

		if (response.error())
		{
			Warning(_log, "%s restoring state failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			rc = false;
		}
	}
	return rc;
}

bool LedDeviceNanoleaf::changeToExternalControlMode()
{
	QJsonDocument resp;
	return changeToExternalControlMode(resp);
}

bool LedDeviceNanoleaf::changeToExternalControlMode(QJsonDocument& resp)
{
	bool success = false;
	Debug(_log, "Set Nanoleaf to External Control (UDP) streaming mode");
	_extControlVersion = EXTCTRLVER_V2;
	//Enable UDP Mode v2

	_restApi->setPath(API_EFFECT);
	httpResponse response = _restApi->put(API_EXT_MODE_STRING_V2);
	if (response.error())
	{
		QString errorReason = QString("Change to external control mode failed with error: '%1'").arg(response.getErrorReason());
		this->setInError(errorReason);
	}
	else
	{
		resp = response.getBody();
		success = true;
	}
	return success;
}

int LedDeviceNanoleaf::write(const std::vector<ColorRgb>& ledValues)
{
	int retVal = 0;

	//
	//    nPanels         2B
	//    panelID         2B
	//    <R> <G> <B>     3B
	//    <W>             1B
	//    tranitionTime   2B
	//
	// Note: Nanoleaf Light Panels (Aurora) now support External Control V2 (tested with FW 3.2.0)

	int udpBufferSize = STREAM_FRAME_PANEL_NUM_SIZE + _panelLedCount * STREAM_FRAME_PANEL_INFO_SIZE;

	QByteArray udpbuffer;
	udpbuffer.resize(udpBufferSize);

	int i = 0;

	// Set number of panels
	qToBigEndian<quint16>(static_cast<quint16>(_panelLedCount), udpbuffer.data() + i);
	i += 2;

	ColorRgb color;

	for (int panelCounter = 0; panelCounter < _panelLedCount; ++panelCounter)
	{
		// Set panelID
		int panelID = _panelIds[panelCounter];
		qToBigEndian<quint16>(static_cast<quint16>(panelID), udpbuffer.data() + i);
		i += 2;

		// Set panel's color LEDs
		if (panelCounter < this->getLedCount()) {
			color = static_cast<ColorRgb>(ledValues.at(panelCounter));
		}
		else
		{
			// Set panels not configured to black
			color = ColorRgb::BLACK;
			DebugIf(verbose3, _log, "[%u] >= panelLedCount [%u] => Set to BLACK", panelCounter, _panelLedCount);
		}

		udpbuffer[i++] = static_cast<char>(color.red);
		udpbuffer[i++] = static_cast<char>(color.green);
		udpbuffer[i++] = static_cast<char>(color.blue);

		// Set white LED
		udpbuffer[i++] = 0; // W not set manually

		// Set transition time
		unsigned char tranitionTime = 1; // currently fixed at value 1 which corresponds to 100ms
		qToBigEndian<quint16>(static_cast<quint16>(tranitionTime), udpbuffer.data() + i);
		i += 2;

		DebugIf(verbose3, _log, "[%u] Color: {%u,%u,%u}", panelCounter, color.red, color.green, color.blue);
	}

	if (verbose3)
	{
		Debug(_log, "UDP-Address [%s], UDP-Port [%u], udpBufferSize[%d], Bytes to send [%d]", QSTRING_CSTR(_address.toString()), _port, udpBufferSize, i);
		Debug(_log, "packet: [%s]", QSTRING_CSTR(toHex(udpbuffer, 64)));
	}

	retVal = writeBytes(udpbuffer);
	return retVal;
}
