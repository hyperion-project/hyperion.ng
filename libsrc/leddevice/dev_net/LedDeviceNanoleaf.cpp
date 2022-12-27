// Local-Hyperion includes
#include "LedDeviceNanoleaf.h"

//std includes
#include <sstream>
#include <iomanip>

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
const char CONFIG_PANEL_START_POS[] = "panelStartPos";

const bool DEFAULT_IS_RESTORE_STATE = true;
const bool DEFAULT_IS_BRIGHTNESS_OVERWRITE = true;
const int BRI_MAX = 100;

// Panel configuration settings
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
const QStringList COLOR_MODES {"hs", "ct", "effect"};
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

const char API_EFFECT_SELECT[] = "select";

//Nanoleaf Control data stream
const int STREAM_FRAME_PANEL_NUM_SIZE = 2;
const int STREAM_FRAME_PANEL_INFO_SIZE = 8;

// Nanoleaf ssdp services
const char SSDP_ID[] = "ssdp:all";
const char SSDP_FILTER_HEADER[] = "ST";
const char SSDP_NANOLEAF[] = "nanoleaf:nl*";
const char SSDP_LIGHTPANELS[] = "nanoleaf_aurora:light";
} //End of constants

// Nanoleaf Panel Shapetypes
enum SHAPETYPES {
	TRIANGLE = 0,
	RHYTM = 1,
	SQUARE = 2,
	CONTROL_SQUARE_PRIMARY = 3,
	CONTROL_SQUARE_PASSIVE = 4,
	POWER_SUPPLY= 5,
	HEXAGON_SHAPES = 7,
	TRIANGE_SHAPES = 8,
	MINI_TRIANGE_SHAPES = 8,
	SHAPES_CONTROLLER = 12
};

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
	  , _startPos(0)
	  , _endPos(0)
	  , _extControlVersion(EXTCTRLVER_V2)
	  , _panelLedCount(0)
{
#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
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
	bool isInitOK {false};

	// Overwrite non supported/required features
	setLatchTime(0);
	setRewriteTime(0);

	if (deviceConfig["rewriteTime"].toInt(0) > 0)
	{
		Info(_log, "Device Nanoleaf does not require rewrites. Refresh time is ignored.");
	}

	DebugIf(verbose,_log, "deviceConfig: [%s]", QString(QJsonDocument(_devConfig).toJson(QJsonDocument::Compact)).toUtf8().constData());

	if ( ProviderUdp::init(deviceConfig) )
	{
		//Set hostname as per configuration and default port
		_hostName = deviceConfig[CONFIG_HOST].toString();
		_port = STREAM_CONTROL_DEFAULT_PORT;
		_apiPort = API_DEFAULT_PORT;
		_authToken = deviceConfig[CONFIG_AUTH_TOKEN].toString();

		_isRestoreOrigState = _devConfig[CONFIG_RESTORE_STATE].toBool(DEFAULT_IS_RESTORE_STATE);
		_isBrightnessOverwrite = _devConfig[CONFIG_BRIGHTNESS_OVERWRITE].toBool(DEFAULT_IS_BRIGHTNESS_OVERWRITE);
		_brightness = _devConfig[CONFIG_BRIGHTNESS].toInt(BRI_MAX);

		Debug(_log, "Hostname/IP       : %s", QSTRING_CSTR(_hostName) );
		Debug(_log, "RestoreOrigState  : %d", _isRestoreOrigState);
		Debug(_log, "Overwrite Brightn.: %d", _isBrightnessOverwrite);
		Debug(_log, "Set Brightness to : %d", _brightness);

		// Read panel organisation configuration
		if (deviceConfig[CONFIG_PANEL_ORDER_TOP_DOWN].isString())
		{
			_topDown = deviceConfig[CONFIG_PANEL_ORDER_TOP_DOWN].toString().toInt() == 0;
		}
		else
		{
			_topDown = deviceConfig[CONFIG_PANEL_ORDER_TOP_DOWN].toInt() == 0;
		}

		if (deviceConfig[CONFIG_PANEL_ORDER_LEFT_RIGHT].isString())
		{
			_leftRight = deviceConfig[CONFIG_PANEL_ORDER_LEFT_RIGHT].toString().toInt() == 0;
		}
		else
		{
			_leftRight = deviceConfig[CONFIG_PANEL_ORDER_LEFT_RIGHT].toInt() == 0;
		}
		_startPos = deviceConfig[CONFIG_PANEL_START_POS].toInt(0);

		isInitOK = true;
	}
	return isInitOK;
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
		this->setInError ( errorReason );
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
		QJsonObject jsonLayout = jsonPanelLayout[PANEL_LAYOUT].toObject();

		int panelNum = jsonLayout[PANEL_NUM].toInt();
		const QJsonArray positionData = jsonLayout[PANEL_POSITIONDATA].toArray();

		std::map<int, std::map<int, int>> panelMap;

		// Loop over all children.
		for(const QJsonValue & value : positionData)
		{
			QJsonObject panelObj = value.toObject();

			int panelId = panelObj[PANEL_ID].toInt();
			int panelX = panelObj[PANEL_POS_X].toInt();
			int panelY = panelObj[PANEL_POS_Y].toInt();
			int panelshapeType = panelObj[PANEL_SHAPE_TYPE].toInt();

			DebugIf(verbose,_log, "Panel [%d] (%d,%d) - Type: [%d]", panelId, panelX, panelY, panelshapeType);

			// Skip Rhythm and Shapes controller panels
			if (panelshapeType != RHYTM && panelshapeType != SHAPES_CONTROLLER)
			{
				panelMap[panelY][panelX] = panelId;
			}
			else
			{	// Reset non support/required features
				Info(_log, "Rhythm/Shape Controller panel skipped.");
			}
		}

		// Travers panels top down
		for (auto posY = panelMap.crbegin(); posY != panelMap.crend(); ++posY)
		{
			// Sort panels left to right
			if (_leftRight)
			{
				for (auto posX = posY->second.cbegin(); posX != posY->second.cend(); ++posX)
				{
					DebugIf(verbose3, _log, "panelMap[%d][%d]=%d", posY->first, posX->first, posX->second);

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
					DebugIf(verbose3, _log, "panelMap[%d][%d]=%d", posY->first, posX->first, posX->second);

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

		this->_panelLedCount = _panelIds.size();
		_devConfig["hardwareLedCount"] = _panelLedCount;

		Debug(_log, "PanelsNum      : %d", panelNum);
		Debug(_log, "PanelLedCount  : %d", _panelLedCount);

		// Check. if enough panels were found.
		int configuredLedCount = this->getLedCount();
		_endPos = _startPos + configuredLedCount - 1;

		Debug(_log, "Sort Top>Down  : %d", _topDown);
		Debug(_log, "Sort Left>Right: %d", _leftRight);
		Debug(_log, "Start Panel Pos: %d", _startPos);
		Debug(_log, "End Panel Pos  : %d", _endPos);

		if (_panelLedCount < configuredLedCount)
		{
			QString errorReason = QString("Not enough panels [%1] for configured LEDs [%2] found!")
									  .arg(_panelLedCount)
									  .arg(configuredLedCount);
			this->setInError(errorReason);
			isInitOK = false;
		}
		else
		{
			if (_panelLedCount > this->getLedCount())
			{
				Info(_log, "%s: More panels [%d] than configured LEDs [%d].", QSTRING_CSTR(this->getActiveDeviceType()), _panelLedCount, configuredLedCount);
			}

			// Check, if start position + number of configured LEDs is greater than number of panels available
			if (_endPos >= _panelLedCount)
			{
				QString errorReason = QString("Start panel [%1] out of range. Start panel position can be max [%2] given [%3] panel available!")
										  .arg(_startPos).arg(_panelLedCount - configuredLedCount).arg(_panelLedCount);

				this->setInError(errorReason);
				isInitOK = false;
			}
		}
	}
	return isInitOK;
}

bool LedDeviceNanoleaf::openRestAPI()
{
	bool isInitOK {true};

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
		if ( openRestAPI() )
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
	deviceList = MdnsBrowser::getInstance().getServicesDiscoveredJson(
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

	DebugIf(verbose,_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return devicesDiscovered;
}

QJsonObject LedDeviceNanoleaf::getProperties(const QJsonObject& params)
{
	DebugIf(verbose,_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());
	QJsonObject properties;

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;
	_authToken = params["token"].toString("");

	Info(_log, "Get properties for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName) );

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if ( openRestAPI() )
		{
			QString filter = params["filter"].toString("");
			_restApi->setPath(filter);

			// Perform request
			httpResponse response = _restApi->get();
			if (response.error())
			{
				Warning(_log, "%s get properties failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			}
			properties.insert("properties", response.getBody().object());
		}

		DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData());
	}
	return properties;
}

void LedDeviceNanoleaf::identify(const QJsonObject& params)
{
	DebugIf(verbose,_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	_authToken = params["token"].toString("");

	Info(_log, "Identify %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName) );

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if ( openRestAPI() )
		{
			_restApi->setPath("identify");

			// Perform request
			httpResponse response = _restApi->put();
			if (response.error())
			{
				Warning(_log, "%s identification failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			}
		}
	}
}

bool LedDeviceNanoleaf::powerOn()
{
	bool on = false;
	if (_isDeviceReady)
	{
		if (changeToExternalControlMode())
		{
			QJsonObject newState;

			QJsonObject onValue { {STATE_VALUE, true} };
			newState.insert(STATE_ON, onValue);

			if ( _isBrightnessOverwrite)
			{
				QJsonObject briValue { {STATE_VALUE, _brightness} };
				newState.insert(STATE_BRI, briValue);
			}

			//Power-on Nanoleaf device
			_restApi->setPath(API_STATE);
			httpResponse response = _restApi->put(newState);
			if (response.error())
			{
				QString errorReason = QString("Power-on request failed with error: '%1'").arg(response.getErrorReason());
				this->setInError ( errorReason );
				on = false;
			} else {
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

		QJsonObject onValue { {STATE_VALUE, false} };
		newState.insert(STATE_ON, onValue);

		//Power-off the Nanoleaf device physically
		_restApi->setPath(API_STATE);
		httpResponse response = _restApi->put(newState);
		if (response.error())
		{
			QString errorReason = QString("Power-off request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError ( errorReason );
			off = false;
		}
	}
	return off;
}

bool LedDeviceNanoleaf::storeState()
{
	bool rc = true;

	if ( _isRestoreOrigState )
	{
		_restApi->setPath(API_STATE);

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

			switch(COLOR_MODES.indexOf(_originalColorMode)) {
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
				if ( responseEffects.error() )
				{
					QString errorReason = QString("Storing device state failed with error: '%1'").arg(responseEffects.getErrorReason());
					setInError(errorReason);
					rc = false;
				}
				else
				{
					QJsonObject effects = responseEffects.getBody().object();
					DebugIf(verbose, _log, "effects: [%s]", QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData() );
					_originalEffect = effects[API_EFFECT_SELECT].toString();
					_originalIsDynEffect = _originalEffect == "*Dynamic*" || _originalEffect == "*Solid*";
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

	if ( _isRestoreOrigState )
	{
		QJsonObject newState;
		switch(COLOR_MODES.indexOf(_originalColorMode)) {
		case 0:
		{	// hs
			QJsonObject hueValue { {STATE_VALUE, _originalHue} };
			newState.insert(STATE_HUE, hueValue);
			QJsonObject satValue { {STATE_VALUE, _originalSat} };
			newState.insert(STATE_SAT, satValue);
			break;
		}
		case 1:
		{	// ct
			QJsonObject ctValue { {STATE_VALUE, _originalCt} };
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
				if ( response.error() )
				{
					Warning (_log, "%s restoring effect failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
				}
			} else {
				Warning (_log, "%s restoring effect failed with error: Cannot restore dynamic or solid effect. Device is switched off", QSTRING_CSTR(_activeDeviceType));
				_originalIsOn = false;
			}
			break;
		}
		default:
			Warning (_log, "%s restoring failed with error: Unknown ColorMode", QSTRING_CSTR(_activeDeviceType));
			rc = false;
		}

		if (!_originalIsDynEffect)
		{
			QJsonObject briValue { {STATE_VALUE, _originalBri} };
			newState.insert(STATE_BRI, briValue);
		}

		QJsonObject onValue { {STATE_VALUE, _originalIsOn} };
		newState.insert(STATE_ON, onValue);

		_restApi->setPath(API_STATE);

		httpResponse response = _restApi->put(newState);

		if ( response.error() )
		{
			Warning (_log, "%s restoring state failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
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
		this->setInError ( errorReason );
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

	//Maintain LED counter independent from PanelCounter
	int ledCounter = 0;
	for (int panelCounter = 0; panelCounter < _panelLedCount; panelCounter++)
	{
		int panelID = _panelIds[panelCounter];

		// Set panels configured
		if (panelCounter >= _startPos && panelCounter <= _endPos) {
			color = static_cast<ColorRgb>(ledValues.at(ledCounter));
			++ledCounter;
		}
		else
		{
			// Set panels not configured to black
			color = ColorRgb::BLACK;
			DebugIf(verbose3, _log, "[%d] >= panelLedCount [%d] => Set to BLACK", panelCounter, _panelLedCount);
		}

		// Set panelID
		qToBigEndian<quint16>(static_cast<quint16>(panelID), udpbuffer.data() + i);
		i += 2;

		// Set panel's color LEDs
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
		Debug( _log, "packet: [%s]", QSTRING_CSTR(toHex(udpbuffer, 64)));
	}

	retVal = writeBytes(udpbuffer);
	return retVal;
}
