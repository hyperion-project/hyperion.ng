// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

// ssdp discover
#include <ssdp/SSDPDiscover.h>

bool verbose = false;

// Configuration settings
static const char CONFIG_ADDRESS[] = "output";
//static const char CONFIG_PORT[] = "port";
static const char CONFIG_USERNAME[] = "username";
static const char CONFIG_CLIENTKEY[] = "clientkey";
static const char CONFIG_BRIGHTNESSFACTOR[] = "brightnessFactor";
static const char CONFIG_TRANSITIONTIME[] = "transitiontime";
static const char CONFIG_BLACK_LIGHTS_TIMEOUT[] = "blackLightsTimeout";
static const char CONFIG_ON_OFF_BLACK[] = "switchOffOnBlack";
static const char CONFIG_RESTORE_STATE[] = "restoreOriginalState";
static const char CONFIG_LIGHTIDS[] = "lightIds";
static const char CONFIG_USE_HUE_ENTERTAINMENT_API[] = "useEntertainmentAPI";
static const char CONFIG_GROUPID[] = "groupId";

static const char CONFIG_VERBOSE[] = "verbose";
static const char CONFIG_BRIGHTNESS_MIN[] = "brightnessMin";
static const char CONFIG_BRIGHTNESS_MAX[] = "brightnessMax";
static const char CONFIG_BRIGHTNESS_THRESHOLD[] = "brightnessThreshold";

static const char CONFIG_SSL_HANDSHAKE_TIMEOUT_MIN[] = "sslHSTimeoutMin";
static const char CONFIG_SSL_HANDSHAKE_TIMEOUT_MAX[] = "sslHSTimeoutMax";
static const char CONFIG_SSL_READ_TIMEOUT[] = "sslReadTimeout";

// Device Data elements
static const char DEV_DATA_BRIDGEID[] = "bridgeid";
static const char DEV_DATA_MODEL[] = "modelid";
static const char DEV_DATA_NAME[] = "name";
//static const char DEV_DATA_MANUFACTURER[] = "manufacturer";
static const char DEV_DATA_FIRMWAREVERSION[] = "swversion";
static const char DEV_DATA_APIVERSION[] = "apiversion";

// Philips Hue OpenAPI URLs
static const int API_DEFAULT_PORT = -1; //Use default port per communication scheme
static const char API_BASE_PATH[] = "/api/%1/";
static const char API_ROOT[] = "";
static const char API_STATE[] = "state";
static const char API_CONFIG[] = "config";
static const char API_LIGHTS[] = "lights";
static const char API_GROUPS[] = "groups";

// List of Group / Stream Information
static const char API_GROUP_NAME[] = "name";
static const char API_GROUP_TYPE[] = "type";
static const char API_GROUP_TYPE_ENTERTAINMENT[] = "Entertainment";
static const char API_STREAM[] = "stream";
static const char API_STREAM_ACTIVE[] = "active";
static const char API_STREAM_ACTIVE_VALUE_TRUE[] = "true";
static const char API_STREAM_ACTIVE_VALUE_FALSE[] = "false";
static const char API_STREAM_OWNER[] = "owner";
static const char API_STREAM_RESPONSE_FORMAT[] = "/%1/%2/%3/%4";

// List of resources
static const char API_XY_COORDINATES[] = "xy";
static const char API_BRIGHTNESS[] = "bri";
//static const char API_SATURATION[] = "sat";
static const char API_TRANSITIONTIME[] = "transitiontime";
static const char API_MODEID[] = "modelid";

// List of State Information
static const char API_STATE_ON[] = "on";
static const char API_STATE_VALUE_TRUE[] = "true";
static const char API_STATE_VALUE_FALSE[] = "false";

// List of Error Information
static const char API_ERROR[] = "error";
static const char API_ERROR_ADDRESS[] = "address";
static const char API_ERROR_DESCRIPTION[] = "description";
static const char API_ERROR_TYPE[] = "type";

// List of Success Information
static const char API_SUCCESS[] = "success";

// Phlips Hue ssdp services
static const char SSDP_ID[] = "upnp:rootdevice";
static const char SSDP_FILTER[] = "(.*)IpBridge(.*)";
static const char SSDP_FILTER_HEADER[] = "SERVER";

// DTLS Connection / SSL / Cipher Suite
static const char API_SSL_SERVER_NAME[] = "Hue";
static const char API_SSL_SEED_CUSTOM[] = "dtls_client";
const int API_SSL_SERVER_PORT = 2100;
const int STREAM_CONNECTION_RETRYS = 5;
const int STREAM_REWRITE_TIME = 20;
const int STREAM_SSL_HANDSHAKE_ATTEMPTS = 5;
const int STREAM_SSL_HANDSHAKE_TIMEOUT_MIN = 400;
const int STREAM_SSL_HANDSHAKE_TIMEOUT_MAX = 1000;
const int STREAM_SSL_READ_TIMEOUT = 0;
const int SSL_CIPHERSUITES[2] = { MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256, 0 };

bool operator ==(const CiColor& p1, const CiColor& p2)
{
	return ((p1.x == p2.x) && (p1.y == p2.y) && (p1.bri == p2.bri));
}

bool operator != (const CiColor& p1, const CiColor& p2)
{
	return !(p1 == p2);
}

CiColor CiColor::rgbToCiColor(double red, double green, double blue, const CiColorTriangle &colorSpace)
{
	double cx;
	double cy;
	double bri;

	if(red + green + blue > 0)
	{
		// Apply gamma correction.
		double r = (red > 0.04045) ? pow((red + 0.055) / (1.0 + 0.055), 2.4) : (red / 12.92);
		double g = (green > 0.04045) ? pow((green + 0.055) / (1.0 + 0.055), 2.4) : (green / 12.92);
		double b = (blue > 0.04045) ? pow((blue + 0.055) / (1.0 + 0.055), 2.4) : (blue / 12.92);

		// Convert to XYZ space.
		double X = r * 0.664511 + g * 0.154324 + b * 0.162028;
		double Y = r * 0.283881 + g * 0.668433 + b * 0.047685;
		double Z = r * 0.000088 + g * 0.072310 + b * 0.986039;

		cx = X / (X + Y + Z);
		cy = Y / (X + Y + Z);

		// RGB to HSV/B Conversion before gamma correction V/B for brightness, not Y from XYZ Space.
		// bri = std::max(std::max(red, green), blue);
		// RGB to HSV/B Conversion after gamma correction V/B for brightness, not Y from XYZ Space.
		bri = std::max(r, std::max(g, b));
	}
	else
	{
		cx = 0.0;
		cy = 0.0;
		bri = 0.0;
	}

	if (std::isnan(cx))
	{
		cx = 0.0;
	}
	if (std::isnan(cy))
	{
		cy = 0.0;
	}
	if (std::isnan(bri))
	{
		bri = 0.0;
	}

	CiColor xy = { cx, cy, bri };

	if(red + green + blue > 0)
	{
		// Check if the given XY value is within the color reach of our lamps.
		if (!isPointInLampsReach(xy, colorSpace))
		{
			// It seems the color is out of reach let's find the closes color we can produce with our lamp and send this XY value out.
			XYColor pAB = getClosestPointToPoint(colorSpace.red, colorSpace.green, xy);
			XYColor pAC = getClosestPointToPoint(colorSpace.blue, colorSpace.red, xy);
			XYColor pBC = getClosestPointToPoint(colorSpace.green, colorSpace.blue, xy);
			// Get the distances per point and see which point is closer to our Point.
			double dAB = getDistanceBetweenTwoPoints(xy, pAB);
			double dAC = getDistanceBetweenTwoPoints(xy, pAC);
			double dBC = getDistanceBetweenTwoPoints(xy, pBC);
			double lowest = dAB;
			XYColor closestPoint = pAB;
			if (dAC < lowest)
			{
				lowest = dAC;
				closestPoint = pAC;
			}
			if (dBC < lowest)
			{
				//lowest = dBC;
				closestPoint = pBC;
			}
			// Change the xy value to a value which is within the reach of the lamp.
			xy.x = closestPoint.x;
			xy.y = closestPoint.y;
		}
	}
	return xy;
}

double CiColor::crossProduct(XYColor p1, XYColor p2)
{
	return p1.x * p2.y - p1.y * p2.x;
}

bool CiColor::isPointInLampsReach(CiColor p, const CiColorTriangle &colorSpace)
{
	bool rc = false;
	XYColor v1 = { colorSpace.green.x - colorSpace.red.x, colorSpace.green.y - colorSpace.red.y };
	XYColor v2 = { colorSpace.blue.x - colorSpace.red.x, colorSpace.blue.y - colorSpace.red.y };
	XYColor  q = { p.x - colorSpace.red.x, p.y - colorSpace.red.y };
	double s = crossProduct(q, v2) / crossProduct(v1, v2);
	double t = crossProduct(v1, q) / crossProduct(v1, v2);
	if ( ( s >= 0.0 ) && ( t >= 0.0 ) && ( s + t <= 1.0 ) )
	{
		rc =  true;
	}
	return rc;
}

XYColor CiColor::getClosestPointToPoint(XYColor a, XYColor b, CiColor p)
{
	XYColor AP = { p.x - a.x, p.y - a.y };
	XYColor AB = { b.x - a.x, b.y - a.y };
	double ab2 = AB.x * AB.x + AB.y * AB.y;
	double ap_ab = AP.x * AB.x + AP.y * AB.y;
	double t = ap_ab / ab2;
	if ( t < 0.0 )
	{
		t = 0.0;
	}
	else if ( t > 1.0 )
	{
		t = 1.0;
	}
	return { a.x + AB.x * t, a.y + AB.y * t };
}

double CiColor::getDistanceBetweenTwoPoints(CiColor p1, XYColor p2)
{
	// Horizontal difference.
	double dx = p1.x - p2.x;
	// Vertical difference.
	double dy = p1.y - p2.y;
	// Absolute value.
	return sqrt(dx * dx + dy * dy);
}

LedDevicePhilipsHueBridge::LedDevicePhilipsHueBridge(const QJsonObject &deviceConfig)
	: ProviderUdpSSL()
	  , _restApi(nullptr)
	  , _apiPort(API_DEFAULT_PORT)
	  , _useHueEntertainmentAPI(false)
	  , _api_major(0)
	  , _api_minor(0)
	  , _api_patch(0)
	  , _isHueEntertainmentReady(false)
{
	_devConfig = deviceConfig;
	_isDeviceReady = false;
}

LedDevicePhilipsHueBridge::~LedDevicePhilipsHueBridge()
{
	if ( _restApi != nullptr )
	{
		delete _restApi;
		_restApi = nullptr;
	}
}

bool LedDevicePhilipsHueBridge::init(const QJsonObject &deviceConfig)
{
	Debug(_log, "");

	_useHueEntertainmentAPI = deviceConfig[CONFIG_USE_HUE_ENTERTAINMENT_API].toBool(false);

	// Overwrite non supported/required features
	_devConfig["latchTime"] = 0;
	if ( deviceConfig["rewriteTime"].toInt(0) > 0 )
	{
		InfoIf ( ( !_useHueEntertainmentAPI ), _log, "Device Philips Hue does not require rewrites. Refresh time is ignored." );
		_devConfig["rewriteTime"] = 0;
	}

	DebugIf( verbose, _log, "deviceConfig: [%s]", QString(QJsonDocument(_devConfig).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	bool isInitOK = false;

	if ( LedDevice::init(deviceConfig) )
	{

		log( "DeviceType", "%s", QSTRING_CSTR( this->getActiveDeviceType() ) );
		log( "LedCount", "%u", this->getLedCount() );
		log( "ColorOrder", "%s", QSTRING_CSTR( this->getColorOrder() ) );
		log( "RefreshTime", "%d", _refreshTimerInterval_ms );
		log( "LatchTime", "%d", this->getLatchTime() );

		//Set hostname as per configuration and_defaultHost default port
		QString address = deviceConfig[ CONFIG_ADDRESS ].toString();

		//If host not configured the init failed
		if ( address.isEmpty() )
		{
			this->setInError("No target hostname nor IP defined");
			return false;
		}
		else
		{
			#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
						QStringList addressparts = address.split(":", Qt::SkipEmptyParts);
			#else
						QStringList addressparts = address.split(":", QString::SkipEmptyParts);
			#endif

			_hostname = addressparts[0];
			log( "Hostname/IP", "%s", QSTRING_CSTR( _hostname ) );

			if ( addressparts.size() > 1 )
			{
				_apiPort = addressparts[1].toInt();
				log( "Port", "%u",  _apiPort );
			}

			_username = deviceConfig[ CONFIG_USERNAME ].toString();

			if ( initRestAPI( _hostname, _apiPort, _username ) )
			{
				if ( initMaps() )
				{
					isInitOK = ProviderUdpSSL::init(_devConfig);
				}
			}
		}
	}
	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::initRestAPI(const QString &hostname, const int port, const QString &token )
{
	Debug(_log, "");
	bool isInitOK = false;

	if ( _restApi == nullptr )
	{
		_restApi = new ProviderRestApi(hostname, port);

		//Base-path is api-path + authentication token (here username)
		_restApi->setBasePath( QString(API_BASE_PATH).arg(token) );

		isInitOK = true;
	}

	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

int LedDevicePhilipsHueBridge::open()
{
	Debug(_log, "");
	int retval = -1;
	_isDeviceReady = false;

	if( _useHueEntertainmentAPI )
	{
		// Open bridge for streaming
		if ( ProviderUdpSSL::open() == 0 )
		{
			// Everything is OK, device is ready
			_isDeviceReady = true;
			retval = 0;
		}
	}
	else
	{
		// Everything is OK, device is ready
		_isDeviceReady = true;
		retval = 0;
	}
	Debug(_log, "[%d]", retval);
	return retval;
}

int LedDevicePhilipsHueBridge::close()
{
	Debug(_log, "");

	_isDeviceReady = false;
	int retval = 0;

	if( _useHueEntertainmentAPI )
	{
		retval = ProviderUdpSSL::close();
	}

	Debug(_log, "[%d]", retval);
	return retval;
}

const int *LedDevicePhilipsHueBridge::getCiphersuites()
{
	return SSL_CIPHERSUITES;
}

void LedDevicePhilipsHueBridge::log(const char* msg, const char* type, ...)
{
	const size_t max_val_length = 1024;
	char val[max_val_length];
	va_list args;
	va_start(args, type);
	vsnprintf(val, max_val_length, type, args);
	va_end(args);
	std::string s = msg;
	int max = 30;
	s.append(max - s.length(), ' ');
	Debug( _log, "%s: %s", s.c_str(), val );
}

QJsonDocument LedDevicePhilipsHueBridge::getAllBridgeInfos()
{
	// Read Groups/ Lights and Light-Ids
	_restApi->setPath(API_ROOT);

	httpResponse response = _restApi->get();
	checkApiError(response.getBody());

	return response.getBody();
}

bool LedDevicePhilipsHueBridge::initMaps()
{
	bool isInitOK = true;

	QJsonDocument doc = getAllBridgeInfos();

	DebugIf( verbose, _log, "doc: [%s]", QString(QJsonDocument(doc).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	if ( this->isInError() )
	{
		isInitOK = false;
	}
	else
	{
		setBridgeConfig( doc );
		if( _useHueEntertainmentAPI )
		{
			setGroupMap( doc );
		}
		setLightsMap( doc );
	}

	return isInitOK;
}

void LedDevicePhilipsHueBridge::setBridgeConfig(const QJsonDocument &doc)
{
	QJsonObject jsonConfigInfo = doc.object()[ API_CONFIG ].toObject();
	if ( verbose )
	{
		std::cout <<  "jsonConfigInfo: [" << QString(QJsonDocument(jsonConfigInfo).toJson(QJsonDocument::Compact)).toUtf8().constData() << "]" << std::endl;
	}

	QString deviceName = jsonConfigInfo[DEV_DATA_NAME].toString();
	_deviceModel = jsonConfigInfo[DEV_DATA_MODEL].toString();
	QString deviceBridgeID = jsonConfigInfo[DEV_DATA_BRIDGEID].toString();
	_deviceFirmwareVersion = jsonConfigInfo[DEV_DATA_FIRMWAREVERSION].toString();
	_deviceAPIVersion = jsonConfigInfo[DEV_DATA_APIVERSION].toString();

	#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
		QStringList apiVersionParts = _deviceAPIVersion.split(".", Qt::SkipEmptyParts);
	#else
		QStringList apiVersionParts = _deviceAPIVersion.split(".", QString::SkipEmptyParts);
	#endif

	if ( !apiVersionParts.isEmpty() )
	{
		_api_major = apiVersionParts[0].toUInt();
		_api_minor = apiVersionParts[1].toUInt();
		_api_patch = apiVersionParts[2].toUInt();

		if ( _api_major > 1 || (_api_major == 1 && _api_minor >= 22) )
		{
			_isHueEntertainmentReady = true;
		}
	}

	if( _useHueEntertainmentAPI )
	{
		DebugIf( !_isHueEntertainmentReady, _log, "Bridge is not Entertainment API Ready - Entertainment API usage was disabled!" );
		_useHueEntertainmentAPI = _isHueEntertainmentReady;
	}

	log( "Bridge Name", "%s", QSTRING_CSTR( deviceName ));
	log( "Model", "%s", QSTRING_CSTR( _deviceModel ));
	log( "Bridge-ID", "%s", QSTRING_CSTR( deviceBridgeID ));
	log( "SoftwareVersion", "%s", QSTRING_CSTR( _deviceFirmwareVersion ));
	log( "API-Version", "%u.%u.%u", _api_major, _api_minor, _api_patch );
	log( "EntertainmentReady", "%d", _isHueEntertainmentReady );
}

void LedDevicePhilipsHueBridge::setLightsMap(const QJsonDocument &doc)
{
	QJsonObject jsonLightsInfo = doc.object()[ API_LIGHTS ].toObject();

	DebugIf(verbose, _log, "jsonLightsInfo: [%s]", QString(QJsonDocument(jsonLightsInfo).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	// Get all available light ids and their values
	QStringList keys = jsonLightsInfo.keys();

	_ledCount = keys.size();
	_lightsMap.clear();

	for ( unsigned int i = 0; i < _ledCount; ++i )
	{
		_lightsMap.insert(keys.at(i).toUInt(), jsonLightsInfo.take(keys.at(i)).toObject());
	}

	if ( getLedCount() == 0 )
	{
		this->setInError( "No light-IDs found at the Philips Hue Bridge" );
	}
	else
	{
		log( "Lights in Bridge found", "%u", getLedCount() );
	}
}

void LedDevicePhilipsHueBridge::setGroupMap(const QJsonDocument &doc)
{
	QJsonObject jsonGroupsInfo = doc.object()[ API_GROUPS ].toObject();

	DebugIf(verbose, _log, "jsonGroupsInfo: [%s]", QString(QJsonDocument(jsonGroupsInfo).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	// Get all available group ids and their values
	QStringList keys = jsonGroupsInfo.keys();

	unsigned int _groupsCount = keys.size();
	_groupsMap.clear();

	for ( unsigned int i = 0; i < _groupsCount; ++i )
	{
		_groupsMap.insert( keys.at(i).toUInt(), jsonGroupsInfo.take(keys.at(i)).toObject() );
	}
}

const QMap<quint16,QJsonObject>& LedDevicePhilipsHueBridge::getLightMap(void)
{
	return _lightsMap;
}

const QMap<quint16,QJsonObject>& LedDevicePhilipsHueBridge::getGroupMap(void)
{
	return _groupsMap;
}

QString LedDevicePhilipsHueBridge::getGroupName(unsigned int groupId)
{
	QString groupName;
	if( _groupsMap.contains( groupId ) )
	{
		QJsonObject group = _groupsMap.value( groupId );
		groupName = group.value( API_GROUP_NAME ).toString().trimmed().replace("\"", "");
	}
	else
	{
		Error(_log, "Group ID %d doesn't exists on this bridge", groupId );
	}
	return groupName;
}

QJsonArray LedDevicePhilipsHueBridge::getGroupLights(unsigned int groupId)
{
	QJsonArray groupLights;
	// search user groupid inside _groupsMap and create light if found
	if( _groupsMap.contains( groupId ) )
	{
		QJsonObject group = _groupsMap.value( groupId );

		if( group.value( API_GROUP_TYPE ) == API_GROUP_TYPE_ENTERTAINMENT )
		{
			QString groupName = getGroupName( groupId );
			groupLights = group.value( API_LIGHTS ).toArray();

			log( "Entertainment Group found", "[%d] %s", groupId, QSTRING_CSTR(groupName) );
			log( "Lights in Group", "%d", groupLights.size() );
			Info(_log, "Entertainment Group [%d] \"%s\" with %d Lights found", groupId, QSTRING_CSTR(groupName), groupLights.size() );
		}
		else
		{
			Error(_log, "Group ID %d is not an entertainment group", groupId );
		}
	}
	else
	{
		Error(_log, "Group ID %d doesn't exists on this bridge", groupId );
	}

	return groupLights;
}

bool LedDevicePhilipsHueBridge::checkApiError(const QJsonDocument &response )
{
	bool apiError = false;
	QString errorReason;

	QString strJson(response.toJson(QJsonDocument::Compact));
	DebugIf(verbose, _log, "Reply: [%s]", strJson.toUtf8().constData() );

	QVariantList rspList = response.toVariant().toList();
	if ( !rspList.isEmpty() )
	{
		QVariantMap map = rspList.first().toMap();
		if ( map.contains( API_ERROR ) )
		{
			// API call failed to execute an error message was returned
			QString errorAddress = map.value(API_ERROR).toMap().value(API_ERROR_ADDRESS).toString();
			QString errorDesc    = map.value(API_ERROR).toMap().value(API_ERROR_DESCRIPTION).toString();
			QString errorType    = map.value(API_ERROR).toMap().value(API_ERROR_TYPE).toString();

			log( "Error Type", "%s", QSTRING_CSTR( errorType ) );
			log( "Error Address", "%s", QSTRING_CSTR( errorAddress ) );
			log( "Error Address Description", "%s", QSTRING_CSTR( errorDesc ) );

			if( errorType != "901" )
			{
				errorReason = QString ("(%1) %2, Resource:%3").arg(errorType, errorDesc, errorAddress);
				this->setInError( errorReason );
				apiError = true;
			}
		}
	}
	return apiError;
}

QJsonDocument LedDevicePhilipsHueBridge::post(const QString& route, const QString& content)
{
	_restApi->setPath(route);

	httpResponse response = _restApi->put(content);
	checkApiError(response.getBody());
	return response.getBody();
}

void LedDevicePhilipsHueBridge::setLightState(const unsigned int lightId, QString state)
{
	DebugIf( verbose, _log, "SetLightState [%u]: %s", lightId, QSTRING_CSTR(state) );
	post( QString("%1/%2/%3").arg( API_LIGHTS ).arg( lightId ).arg( API_STATE ), state );
}

QJsonDocument LedDevicePhilipsHueBridge::getGroupState(const unsigned int groupId)
{
	_restApi->setPath( QString("%1/%2").arg( API_GROUPS ).arg( groupId ) );
	httpResponse response = _restApi->get();
	checkApiError(response.getBody());
	return response.getBody();
}

QJsonDocument LedDevicePhilipsHueBridge::setGroupState(const unsigned int groupId, bool state)
{
	QString active = state ? API_STREAM_ACTIVE_VALUE_TRUE : API_STREAM_ACTIVE_VALUE_FALSE;
	return post( QString("%1/%2").arg( API_GROUPS ).arg( groupId ), QString("{\"%1\":{\"%2\":%3}}").arg( API_STREAM, API_STREAM_ACTIVE, active ) );
}

bool LedDevicePhilipsHueBridge::isStreamOwner(const QString &streamOwner)
{
	return ( streamOwner != "" && streamOwner == _username );
}

const std::set<QString> PhilipsHueLight::GAMUT_A_MODEL_IDS =
	{ "LLC001", "LLC005", "LLC006", "LLC007", "LLC010", "LLC011", "LLC012", "LLC013", "LLC014", "LST001" };
const std::set<QString> PhilipsHueLight::GAMUT_B_MODEL_IDS =
	{ "LCT001", "LCT002", "LCT003", "LCT007", "LLM001" };
const std::set<QString> PhilipsHueLight::GAMUT_C_MODEL_IDS =
	{ "LCA001", "LCA002", "LCA003", "LCG002", "LCP001", "LCP002", "LCT010", "LCT011", "LCT012", "LCT014", "LCT015", "LCT016", "LCT024", "LLC020", "LST002" };

PhilipsHueLight::PhilipsHueLight(Logger* log, unsigned int id, QJsonObject values, unsigned int ledidx)
	: _log(log)
	  , _id(id)
	  , _ledidx(ledidx)
	  , _on(false)
	  , _transitionTime(0)
	  , _colorBlack({0.0, 0.0, 0.0})
	  , _modelId(values[API_MODEID].toString().trimmed().replace("\"", ""))
{
	// Find id in the sets and set the appropriate color space.
	if (GAMUT_A_MODEL_IDS.find(_modelId) != GAMUT_A_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut A", QSTRING_CSTR(_modelId), id );
		_colorSpace.red		= {0.704, 0.296};
		_colorSpace.green	= {0.2151, 0.7106};
		_colorSpace.blue	= {0.138, 0.08};
		_colorBlack 		= {0.138, 0.08, 0.0};
	}
	else if (GAMUT_B_MODEL_IDS.find(_modelId) != GAMUT_B_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut B", QSTRING_CSTR(_modelId), id );
		_colorSpace.red 	= {0.675, 0.322};
		_colorSpace.green	= {0.409, 0.518};
		_colorSpace.blue 	= {0.167, 0.04};
		_colorBlack 		= {0.167, 0.04, 0.0};
	}
	else if (GAMUT_C_MODEL_IDS.find(_modelId) != GAMUT_C_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut C", QSTRING_CSTR(_modelId), id );
		_colorSpace.red		= {0.6915, 0.3083};
		_colorSpace.green	= {0.17, 0.7};
		_colorSpace.blue 	= {0.1532, 0.0475};
		_colorBlack 		= {0.1532, 0.0475, 0.0};
	}
	else
	{
		Warning(_log, "Did not recognize model id %s of light ID %d", QSTRING_CSTR(_modelId), id );
		_colorSpace.red 	= {1.0, 0.0};
		_colorSpace.green 	= {0.0, 1.0};
		_colorSpace.blue 	= {0.0, 0.0};
		_colorBlack 		= {0.0, 0.0, 0.0};
	}

	saveOriginalState(values);

	_lightname = values["name"].toString().trimmed().replace("\"", "");
	Info(_log, "Light ID %d (\"%s\", LED index \"%d\") created", id, QSTRING_CSTR(_lightname), ledidx );
}

PhilipsHueLight::~PhilipsHueLight()
{
	DebugIf(verbose, _log, "Light ID %d (\"%s\", LED index \"%d\") deconstructed", _id, QSTRING_CSTR(_lightname), _ledidx );
}

unsigned int PhilipsHueLight::getId() const
{
	return _id;
}

QString PhilipsHueLight::getOriginalState() const
{
	return _originalState;
}

void PhilipsHueLight::saveOriginalState(const QJsonObject& values)
{
	// Get state object values which are subject to change.
	if (!values[API_STATE].toObject().contains("on"))
	{
		Error(_log, "Got invalid state object from light ID %d", _id );
	}
	QJsonObject lState = values[API_STATE].toObject();
	_originalStateJSON = lState;

	QJsonObject state;
	state["on"] = lState["on"];
	_originalColor = _colorBlack;
	QString c;
	if (state[API_STATE_ON].toBool())
	{
		state[API_XY_COORDINATES] = lState[API_XY_COORDINATES];
		state[API_BRIGHTNESS] = lState[API_BRIGHTNESS];
		_on = true;
		_color = {
			state[API_XY_COORDINATES].toArray()[0].toDouble(),
			state[API_XY_COORDINATES].toArray()[1].toDouble(),
			state[API_BRIGHTNESS].toDouble() / 254.0
		};
		_originalColor = _color;
		c = QString("{ \"%1\": [%2, %3], \"%4\": %5 }").arg(API_XY_COORDINATES).arg(_originalColor.x, 0, 'd', 4).arg(_originalColor.y, 0, 'd', 4).arg(API_BRIGHTNESS).arg((_originalColor.bri * 254.0), 0, 'd', 4);
		DebugIf(verbose, _log, "OriginalColor state on: %s", QSTRING_CSTR(c));
		_transitionTime = values[API_STATE].toObject()[API_TRANSITIONTIME].toInt();
	}
	//Determine the original state.
	_originalState = QJsonDocument(state).toJson(QJsonDocument::JsonFormat::Compact).trimmed();
}

void PhilipsHueLight::setOnOffState(bool on)
{
	this->_on = on;
}

void PhilipsHueLight::setTransitionTime(unsigned int transitionTime)
{
	this->_transitionTime = transitionTime;
}

void PhilipsHueLight::setColor(const CiColor& color)
{
	this->_color = color;
}

bool PhilipsHueLight::getOnOffState() const
{
	return _on;
}

unsigned int PhilipsHueLight::getTransitionTime() const
{
	return _transitionTime;
}

CiColor PhilipsHueLight::getColor() const
{
	return _color;
}

CiColorTriangle PhilipsHueLight::getColorSpace() const
{
	return _colorSpace;
}

LedDevicePhilipsHue::LedDevicePhilipsHue(const QJsonObject& deviceConfig)
	: LedDevicePhilipsHueBridge(deviceConfig)
	  , _switchOffOnBlack(false)
	  , _brightnessFactor(1.0)
	  , _transitionTime(1)
	  , _lightStatesRestored(false)
	  , _isInitLeds(false)
	  , _lightsCount(0)
	  , _groupId(0)
	  , _brightnessMin(0.0)
	  , _brightnessMax(1.0)
	  , _allLightsBlack(false)
	  , _blackLightsTimer(nullptr)
	  , _blackLightsTimeout(15000)
	  , _brightnessThreshold(0.0)
	  , _handshake_timeout_min(STREAM_SSL_HANDSHAKE_TIMEOUT_MIN)
	  , _handshake_timeout_max(STREAM_SSL_HANDSHAKE_TIMEOUT_MAX)
	  , _ssl_read_timeout(STREAM_SSL_READ_TIMEOUT)
	  , _stopConnection(false)
	  , start_retry_left(3)
	  , stop_retry_left(3)
{
	_devConfig = deviceConfig;
	_isDeviceReady = false;

	_activeDeviceType = deviceConfig["type"].toString("UNSPECIFIED").toLower();
}

LedDevice* LedDevicePhilipsHue::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePhilipsHue(deviceConfig);
}

LedDevicePhilipsHue::~LedDevicePhilipsHue()
{
	if ( _blackLightsTimer != nullptr )
	{
		_blackLightsTimer->deleteLater();
	}
}

bool LedDevicePhilipsHue::init(const QJsonObject &deviceConfig)
{
	Debug(_log, "");
	verbose = deviceConfig[CONFIG_VERBOSE].toBool(false);

	bool isInitOK = LedDevicePhilipsHueBridge::init(deviceConfig);

	if ( isInitOK )
	{
		// Initiatiale LedDevice configuration and execution environment
		_switchOffOnBlack       = _devConfig[CONFIG_ON_OFF_BLACK].toBool(true);
		_blackLightsTimeout     = _devConfig[CONFIG_BLACK_LIGHTS_TIMEOUT].toInt(15000);
		_brightnessFactor       = _devConfig[CONFIG_BRIGHTNESSFACTOR].toDouble(1.0);
		_transitionTime         = _devConfig[CONFIG_TRANSITIONTIME].toInt(1);
		_isRestoreOrigState     = _devConfig[CONFIG_RESTORE_STATE].toBool(true);
		_groupId                = _devConfig[CONFIG_GROUPID].toInt(0);
		_brightnessMin          = _devConfig[CONFIG_BRIGHTNESS_MIN].toDouble(0.0);
		_brightnessMax          = _devConfig[CONFIG_BRIGHTNESS_MAX].toDouble(1.0);
		_brightnessThreshold    = _devConfig[CONFIG_BRIGHTNESS_THRESHOLD].toDouble(0.0);
		_handshake_timeout_min  = _devConfig[CONFIG_SSL_HANDSHAKE_TIMEOUT_MIN].toInt(STREAM_SSL_HANDSHAKE_TIMEOUT_MIN);
		_handshake_timeout_max  = _devConfig[CONFIG_SSL_HANDSHAKE_TIMEOUT_MAX].toInt(STREAM_SSL_HANDSHAKE_TIMEOUT_MAX);
		_ssl_read_timeout       = _devConfig[CONFIG_SSL_READ_TIMEOUT].toInt(STREAM_SSL_READ_TIMEOUT);

		if( _brightnessMin < 0.0 ) _brightnessMin = 0.0;
		if( _brightnessMax > 1.0 ) _brightnessMax = 1.0;
		if( _brightnessThreshold < 0.0 ) _brightnessThreshold = 0.0;
		if( _brightnessThreshold > 1.0 ) _brightnessThreshold = 1.0;

		if( _handshake_timeout_min <= 0 ) _handshake_timeout_min = 1;

		log( "Off on Black", "%d", _switchOffOnBlack );
		log( "Brightness Factor", "%f", _brightnessFactor );
		log( "Transition Time", "%d", _transitionTime );
		log( "Restore Original State", "%d", _isRestoreOrigState );
		log( "Use Hue Entertainment API", "%d", _useHueEntertainmentAPI );

		if( _useHueEntertainmentAPI )
		{
			log( "Entertainment API Group-ID", "%d", _groupId );
			log( "Signal Timeout on Black", "%dms", _blackLightsTimeout );
			log( "Brightness Min", "%f", _brightnessMin );
			log( "Brightness Max", "%f", _brightnessMax );
			log( "Brightness Threshold", "%f", _brightnessThreshold );

			if( _groupId == 0 )
			{
				log( "Group-ID is invalid", "%d", _groupId );
				_useHueEntertainmentAPI = false;
			}
		}

		isInitOK =  initLeds();
	}
	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

bool LedDevicePhilipsHue::setLights()
{
	Debug(_log, "");
	bool isInitOK = true;

	_lightIds.clear();

	QJsonArray lArray;

	if( _useHueEntertainmentAPI && _groupId > 0 )
	{
		lArray = getGroupLights( _groupId );
	}

	if( lArray.empty() )
	{
		if( _useHueEntertainmentAPI )
		{
			_useHueEntertainmentAPI = false;
			Debug(_log, "Group-ID [%u] is not usable - Entertainment API usage was disabled!", _groupId );
		}
		lArray = _devConfig[ CONFIG_LIGHTIDS ].toArray();
	}

	QString lightIDStr;

	if( !lArray.empty() )
	{
		for (const auto id : lArray)
		{
			unsigned int lightId = id.toString().toUInt();
			if( lightId > 0 )
			{
				if(std::find(_lightIds.begin(), _lightIds.end(), lightId) == _lightIds.end())
				{
					_lightIds.emplace_back(lightId);
					if(!lightIDStr.isEmpty()) lightIDStr.append(", ");
					lightIDStr.append(QString::number(lightId));
				}
			}
		}
		std::sort( _lightIds.begin(), _lightIds.end() );
	}

	unsigned int configuredLightsCount = static_cast<unsigned int>(_lightIds.size());

	log( "Light-IDs configured", "%d", configuredLightsCount );

	if ( configuredLightsCount == 0 )
	{
		this->setInError( "No light-IDs configured" );
		isInitOK = false;
	}
	else
	{
		log( "Light-IDs", "%s", QSTRING_CSTR( lightIDStr ) );
		isInitOK = updateLights( getLightMap() );
	}

	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

bool LedDevicePhilipsHue::initLeds()
{
	Debug(_log, "");
	bool isInitOK = false;

	if ( !this->isInError() )
	{
		if( setLights() )
		{
			if( _useHueEntertainmentAPI )
			{
				_groupName = getGroupName( _groupId );
				_devConfig["latchTime"]      = 0;
				_devConfig["host"]           = _hostname;
				_devConfig["sslport"]        = API_SSL_SERVER_PORT;
				_devConfig["servername"]     = API_SSL_SERVER_NAME;
				_devConfig["rewriteTime"]    = STREAM_REWRITE_TIME;
				_devConfig["psk"]            = _devConfig[ CONFIG_CLIENTKEY ].toString();
				_devConfig["psk_identity"]   = _devConfig[ CONFIG_USERNAME ].toString();
				_devConfig["seed_custom"]    = API_SSL_SEED_CUSTOM;
				_devConfig["retry_left"]     = STREAM_CONNECTION_RETRYS;
				_devConfig["hs_attempts"]    = STREAM_SSL_HANDSHAKE_ATTEMPTS;
				_devConfig["hs_timeout_min"] = _handshake_timeout_min;
				_devConfig["hs_timeout_max"] = _handshake_timeout_max;
				_devConfig["read_timeout"]   = _ssl_read_timeout;

				isInitOK = ProviderUdpSSL::init( _devConfig );

				if( isInitOK )
				{
					if ( _blackLightsTimer == nullptr )
					{
						_blackLightsTimer = new QTimer(this);
						connect( _blackLightsTimer, &QTimer::timeout, this, &LedDevicePhilipsHue::noSignalTimeout );
					}
				}
			}
			else
			{
				// adapt latchTime to count of user lightIds (bridge 10Hz max overall)
				setLatchTime( static_cast<int>( 100 * getLightsCount() ) );
				isInitOK = true;
			}
			_isInitLeds = true;
		}
		else
		{
			isInitOK = false;
		}
	}
	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

bool LedDevicePhilipsHue::updateLights(QMap<quint16, QJsonObject> map)
{
	Debug(_log, "");
	bool isInitOK = true;

	// search user lightid inside map and create light if found
	_lights.clear();

	if(!_lightIds.empty())
	{
		unsigned int ledidx = 0;
		_lights.reserve(_lightIds.size());
		for(const auto id : _lightIds)
		{
			if (map.contains(id))
			{
				_lights.emplace_back(_log, id, map.value(id), ledidx);
			}
			else
			{
				Warning(_log, "Configured light-ID %d is not available at this bridge", id );
			}
			ledidx++;
		}
	}

	unsigned int lightsCount = static_cast<unsigned int>(_lights.size());

	setLightsCount( lightsCount );

	if( lightsCount == 0 )
	{
		Debug(_log, "No usable lights found!" );
		isInitOK = false;
	}

	Debug(_log, "[%d]", isInitOK);
	return isInitOK;
}

bool LedDevicePhilipsHue::openStream()
{
	bool isInitOK = false;

	start_retry_left = 3;

	bool streamState = getStreamGroupState();

	if ( !this->isInError() )
	{
		// stream is already active
		if( streamState )
		{
			// if same owner stop stream
			if(isStreamOwner(_streamOwner))
			{
				Debug(_log, "Group: \"%s\" [%u] is in use, try to stop stream", QSTRING_CSTR(_groupName), _groupId );

				if( stopStream() )
				{
					Debug(_log, "Stream successful stopped");
					//Restore Philips Hue devices state
					restoreState();
					isInitOK = startStream();
				}
				else
				{
					Error(_log, "Group: \"%s\" [%u] couldn't stop by user: \"%s\" - Entertainment API not usable", QSTRING_CSTR( _groupName ), _groupId, QSTRING_CSTR( _streamOwner ) );
				}
			}
			else
			{
				Error(_log, "Group: \"%s\" [%u] is in use and owned by other user: \"%s\" - Entertainment API not usable", QSTRING_CSTR(_groupName), _groupId, QSTRING_CSTR(_streamOwner));
			}
		}
		else
		{
			isInitOK = startStream();
		}
	}

	if( isInitOK )
	{
		// open UDP SSL Connection
		isInitOK = ProviderUdpSSL::initNetwork();

		if( isInitOK )
		{
			Info(_log, "Philips Hue Entertaiment API successful connected! Start Streaming." );
			_allLightsBlack = true;
			noSignalDetection();
		}
		else
		{
			Error(_log, "Philips Hue Entertaiment API not connected!" );
		}
	}
	else
	{
		Error(_log, "Philips Hue Entertaiment API could not initialisized!" );
	}

	return isInitOK;
}

bool LedDevicePhilipsHue::startStream()
{
	Debug(_log, "Start entertainment stream");

	if ( setStreamGroupState( true ) )
	{
		start_retry_left = 3;
		return true;
	}
	else
	{
		if ( !this->isInError() )
		{
			QThread::msleep(500);
			bool streamState = getStreamGroupState();

			if ( !this->isInError() )
			{
				// stream is not active
				if( !streamState )
				{
					return ( start_retry_left-- > 0 ) ? startStream() : false;
				}
			}
		}
	}

	return false;
}

bool LedDevicePhilipsHue::stopStream()
{
	// TODO: Is is right that the sslconnection is closed before changing the stream state
	ProviderUdpSSL::closeSSLConnection();

	if ( setStreamGroupState( false ) )
	{
		stop_retry_left = 3;
		return true;
	}
	else
	{
		if ( !this->isInError() )
		{
			QThread::msleep(500);
			bool streamState = getStreamGroupState();

			if ( !this->isInError() )
			{
				// stream is still active
				if( streamState )
				{
					return (stop_retry_left-- > 0) ? stopStream() : false;
				}
			}
		}
	}

	return false;
}

bool LedDevicePhilipsHue::getStreamGroupState()
{
	QJsonDocument doc = getGroupState( _groupId );

	if ( !this->isInError() )
	{
		QJsonObject obj = doc.object()[ API_STREAM ].toObject();

		if( obj.isEmpty() )
		{
			this->setInError( "no Streaming Infos in Group found" );
		}
		else
		{
			_streamOwner = obj.value( API_STREAM_OWNER ).toString();
			bool streamState = obj.value( API_STREAM_ACTIVE ).toBool();
			return streamState;
		}
	}

	return false;
}

bool LedDevicePhilipsHue::setStreamGroupState(bool state)
{
	QString active = state ? API_STREAM_ACTIVE_VALUE_TRUE : API_STREAM_ACTIVE_VALUE_FALSE;

	QJsonDocument doc = setGroupState( _groupId, state );

	QVariant rsp = doc.toVariant();
	QVariantMap map = rsp.toList().first().toMap();

	if ( !map.contains( API_SUCCESS ) )
	{
		this->setInError( QString("set stream to %1: Neither error nor success contained in Bridge response...").arg( active ) );
	}
	else
	{
		QString valueName = QString( API_STREAM_RESPONSE_FORMAT ).arg( API_GROUPS ).arg( _groupId ).arg( API_STREAM, API_STREAM_ACTIVE );
		if(!map.value( API_SUCCESS ).toMap().value( valueName ).isValid())
		{
			this->setInError( QString("set stream to %1: Bridge response is not Valid").arg( active ) );
		}
		else
		{
			bool groupStreamState = map.value( API_SUCCESS ).toMap().value( valueName ).toBool();
			return ( groupStreamState == state );
		}
	}

	return false;
}

QByteArray LedDevicePhilipsHue::prepareStreamData()
{
	static const uint8_t HEADER[] =
	{
		'H', 'u', 'e', 'S', 't', 'r', 'e', 'a', 'm', //protocol
		0x01, 0x00, //version 1.0
		0x01, //sequence number 1
		0x00, 0x00, //Reserved write 0’s
		0x01, //xy Brightness
		0x00, // Reserved, write 0’s
	};

	static const uint8_t PAYLOAD_PER_LIGHT[] =
	{
		0x01, 0x00, 0x06, //light ID
		//color: 16 bpc
		0xff, 0xff,
		0xff, 0xff,
		0xff, 0xff,
		/*
		(message.R >> 8) & 0xff, message.R & 0xff,
		(message.G >> 8) & 0xff, message.G & 0xff,
		(message.B >> 8) & 0xff, message.B & 0xff
		*/
	};

	QByteArray msg;
	msg.reserve(static_cast<int>(sizeof(HEADER) + sizeof(PAYLOAD_PER_LIGHT) * _lights.size()));
	msg.append((char*)HEADER, sizeof(HEADER));

	for (PhilipsHueLight& light : _lights)
	{
		CiColor lightC = light.getColor();
		quint64 R = lightC.x * 0xffff;
		quint64 G = lightC.y * 0xffff;
		quint64 B = lightC.bri * 0xffff;
		unsigned int id = light.getId();
		const uint8_t payload[] = {
			0x00, 0x00, static_cast<uint8_t>(id),
			static_cast<uint8_t>((R >> 8) & 0xff), static_cast<uint8_t>(R & 0xff),
			static_cast<uint8_t>((G >> 8) & 0xff), static_cast<uint8_t>(G & 0xff),
			static_cast<uint8_t>((B >> 8) & 0xff), static_cast<uint8_t>(B & 0xff)
		};
		msg.append((char*)payload, sizeof(payload));
	}

	return msg;
}

void LedDevicePhilipsHue::stop()
{
	Debug(_log, "");

	// TODO: Check, if anything specific before switch-off during stopping is required, otherwise remove
	// LedDevicePhilipsHueBridge::stop();

	Debug(_log, "LedDevicePhilipsHue::stop() [void]");
}

int LedDevicePhilipsHue::open()
{
	Debug(_log, "");
	int retval = -1;
	_isDeviceReady = false;

	if( _useHueEntertainmentAPI )
	{
		if ( openStream() )
		{
			// Everything is OK, device is ready
			_isDeviceReady = true;
			retval = 0;
		}
		else
		{
			// TODO: Stop device (or fallback to classic mode) - suggest to stop device to meet user expectation
			//_useHueEntertainmentAPI = false; -to be removed, if 1
			// Everything is OK, device is ready
		}
	}
	else
	{
		// Classic mode, everything is OK, device is ready
		_isDeviceReady = true;
		retval = 0;
	}
	Debug(_log, "[%d]", retval);
	return retval;
}

int LedDevicePhilipsHue::close()
{
	Debug(_log, "");
	int retval = -1;

	retval = LedDevicePhilipsHueBridge::close();

	Debug(_log, "[%d]", retval);
	return retval;
}

bool LedDevicePhilipsHue::switchOff()
{
	Debug(_log, "");

	this->stopBlackTimeoutTimer();

	stop_retry_left = 3;
	stopStream();

	return LedDevicePhilipsHueBridge::switchOff();
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> & ledValues)
{
	// lights will be empty sometimes
	if( _lights.empty() ) return -1;

	// more lights then leds, stop always
	if( ledValues.size() < getLightsCount() )
	{
		Error(_log, "More light-IDs configured than leds, each light-ID requires one led!" );
		return -1;
	}

	writeSingleLights( ledValues );

	if( _useHueEntertainmentAPI && !noSignalDetection() && _isInitLeds ) writeStream();

	return 0;
}

void LedDevicePhilipsHue::noSignalTimeout()
{
	Debug(_log, "No Signal (timeout: %sms), only black color detected - stop stream for \"%s\" [%u]", QSTRING_CSTR( QString::number( _blackLightsTimer->remainingTime() ) ), QSTRING_CSTR(_groupName), _groupId );
	_stopConnection = true;
	switchOff();
}

void LedDevicePhilipsHue::stopBlackTimeoutTimer()
{
	if ( _blackLightsTimer != nullptr && _blackLightsTimer->isActive() )
	{
		_blackLightsTimer->stop();
	}
}

bool LedDevicePhilipsHue::noSignalDetection()
{
	if( _allLightsBlack )
	{
		if( !_stopConnection && _isInitLeds )
		{
			if ( !_blackLightsTimer->isActive() )
			{
				DebugIf( verbose, _log, "No Signal detected - timeout timer started" );
				_blackLightsTimer->start( ( _blackLightsTimeout + 500 ) );
			}
		}
	}
	else
	{
		if ( _blackLightsTimer->isActive() )
		{
			DebugIf( verbose, _log, "Signal detected - timeout timer stopped" );
			this->stopBlackTimeoutTimer();
		}

		if( _stopConnection )
		{
			_stopConnection = false;
			Debug(_log, "Signal detected - restart stream for \"%s\" [%u]", QSTRING_CSTR(_groupName), _groupId );
			switchOn();
		}
	}
	return _stopConnection;
}

int LedDevicePhilipsHue::writeSingleLights(const std::vector<ColorRgb>& ledValues)
{
	// Iterate through lights and set colors.
	unsigned int idx = 0;
	unsigned int blackCounter = 0;
	for ( PhilipsHueLight& light : _lights )
	{
		// Get color.
		ColorRgb color = ledValues.at(idx);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		CiColor xy = CiColor::rgbToCiColor(color.red / 255.0, color.green / 255.0, color.blue / 255.0, light.getColorSpace());

		if( _useHueEntertainmentAPI )
		{
			this->setColor(light, xy);
			if( xy.bri >= 0.0 && xy.bri <= _brightnessThreshold )
			{
				blackCounter++;
			}
		}
		else
		{
			if ( _switchOffOnBlack && xy.bri == 0.0 )
			{
				this->setOnOffState( light, false );
			}
			else
			{
				// Write color if color has been changed.
				this->setState( light, true, xy );
			}
		}
		idx++;
	}

	if( _useHueEntertainmentAPI )
	{
		_allLightsBlack = ( blackCounter == _lightsCount );
	}

	return 0;
}

void LedDevicePhilipsHue::writeStream()
{
	QByteArray streamData = prepareStreamData();
	writeBytes( streamData.size(), reinterpret_cast<unsigned char *>( streamData.data() ) );
}

void LedDevicePhilipsHue::setOnOffState(PhilipsHueLight& light, bool on)
{
	if (light.getOnOffState() != on)
	{
		light.setOnOffState( on );
		QString state = on ? API_STATE_VALUE_TRUE : API_STATE_VALUE_FALSE;
		setLightState( light.getId(), QString("{\"%1\": %2 }").arg( API_STATE_ON, state ) );
	}
}

void LedDevicePhilipsHue::setTransitionTime(PhilipsHueLight& light)
{
	if (light.getTransitionTime() != _transitionTime)
	{
		light.setTransitionTime( _transitionTime );
		setLightState( light.getId(), QString("{\"%1\": %2 }").arg( API_TRANSITIONTIME ).arg( _transitionTime ) );
	}
}

void LedDevicePhilipsHue::setColor(PhilipsHueLight& light, CiColor& color)
{
	if ( light.getColor() != color )
	{
		if( !_useHueEntertainmentAPI )
		{
			const int bri = qRound(qMin(254.0, _brightnessFactor * qMax(1.0, color.bri * 254.0)));
			QString stateCmd = QString("\"%1\":[%2,%3],\"%4\":%5").arg( API_XY_COORDINATES ).arg( color.x, 0, 'd', 4 ).arg( color.y, 0, 'd', 4 ).arg( API_BRIGHTNESS ).arg( bri );
			setLightState( light.getId(), stateCmd );
		}
		else
		{
			color.bri = ( qMin( _brightnessMax, _brightnessFactor * qMax( _brightnessMin, color.bri ) ) );
			//if(color.x == 0.0 && color.y == 0.0) color = colorBlack;
		}
		light.setColor( color );
	}
}

void LedDevicePhilipsHue::setState(PhilipsHueLight& light, bool on, const CiColor& color)
{
	QString stateCmd;

	if ( light.getOnOffState() != on )
	{
		light.setOnOffState( on );
		QString state = on ? API_STATE_VALUE_TRUE : API_STATE_VALUE_FALSE;
		stateCmd += QString("\"%1\":%2,").arg( API_STATE_ON, state );
	}

	if ( light.getTransitionTime() != _transitionTime )
	{
		light.setTransitionTime( _transitionTime );
		stateCmd += QString("\"%1\":%2,").arg( API_TRANSITIONTIME ).arg( _transitionTime );
	}

	const int bri = qRound( qMin( 254.0, _brightnessFactor * qMax( 1.0, color.bri * 254.0 ) ) );
	if ( light.getColor() != color )
	{
		light.setColor( color );
		stateCmd += QString("\"%1\":[%2,%3],\"%4\":%5").arg( API_XY_COORDINATES ).arg( color.x, 0, 'd', 4 ).arg( color.y, 0, 'd', 4 ).arg( API_BRIGHTNESS ).arg( bri );

	}

	if ( !stateCmd.isEmpty() )
	{
		setLightState( light.getId(), "{" + stateCmd + "}" );
	}
}

void LedDevicePhilipsHue::setLightsCount( unsigned int lightsCount )
{
	_lightsCount = lightsCount;
}

bool LedDevicePhilipsHue::powerOn()
{
	Debug(_log, "");
	if ( _isDeviceReady)
	{
		// TODO: Question: Not clear, if setstream state on will turn of the lights
		// or do they need to be turned off classically?
		if ( !_useHueEntertainmentAPI )
		{
			//Switch off Philips Hue devices physically
			for ( PhilipsHueLight& light : _lights )
			{
				setOnOffState( light, true );
			}
		}
	}
	return true;
}

bool LedDevicePhilipsHue::powerOff()
{
	Debug(_log, "");
	if ( _isDeviceReady)
	{
		// TODO: Question: Not clear, if setstream state off will turn of the lights
		// or do they need to be turned off classically
		if ( !_useHueEntertainmentAPI )
		{
			//Switch off Philips Hue devices physically
			for ( PhilipsHueLight& light : _lights )
			{
				setOnOffState( light, false );
			}
		}
	}
	return true;
}

bool LedDevicePhilipsHue::storeState()
{
	Debug(_log, "");
	bool rc = true;

	if ( _isRestoreOrigState )
	{
		// Save device's original state
		//_orignalStateValues = get device's state;

		// TODO: Move saveOriginalState out of the HueLight constructor,
		// as the light state may have change since last close and needs to be stored again before reopen
	}

	Debug(_log, "[%d]", rc);
	return rc;
}

bool LedDevicePhilipsHue::restoreState()
{
	Debug(_log, "");
	bool rc = true;

	if ( _isRestoreOrigState && !_lightStatesRestored )
	{
		// Restore device's original state
		_lightStatesRestored = true;

		if( !_lightIds.empty() )
		{
			for ( PhilipsHueLight& light : _lights )
			{
				setLightState( light.getId(),light.getOriginalState() );
			}
		}
	}
	Debug(_log, "[%d]", rc);
	return rc;
}

QJsonObject LedDevicePhilipsHue::discover()
{
	Debug(_log, "");

	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

	// Discover Devices
	SSDPDiscover discover;

	discover.skipDuplicateKeys(false);
	discover.setSearchFilter(SSDP_FILTER, SSDP_FILTER_HEADER);
	QString searchTarget = SSDP_ID;

	if ( discover.discoverServices(searchTarget) > 0 )
	{
		deviceList = discover.getServicesDiscoveredJson();
	}

	devicesDiscovered.insert("devices", deviceList);
	Debug(_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return devicesDiscovered;
}

QJsonObject LedDevicePhilipsHue::getProperties(const QJsonObject& params)
{
	QJsonObject properties;

	// Get Phillips-Bridge device properties
	QString host = params["host"].toString("");
	if ( !host.isEmpty() )
	{
		QString username = params["user"].toString("");
		QString filter = params["filter"].toString("");

		// Resolve hostname and port (or use default API port)
		#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
				QStringList addressparts = host.split(":", Qt::SkipEmptyParts);
		#else
				QStringList addressparts = host.split(":", QString::SkipEmptyParts);
		#endif

		QString apiHost = addressparts[0];
		int apiPort;

		if ( addressparts.size() > 1)
			apiPort = addressparts[1].toInt();
		else
			apiPort   = API_DEFAULT_PORT;

		initRestAPI(apiHost, apiPort, username);
		_restApi->setPath(filter);

		// Perform request
		httpResponse response = _restApi->get();
		if ( response.error() )
		{
			Warning (_log, "%s get properties failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		}

		// Perform request
		properties.insert("properties", response.getBody().object());
	}
	return properties;
}

void LedDevicePhilipsHue::identify(const QJsonObject& params)
{
	Debug(_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	QJsonObject properties;

	// Identify Phillips-Bridge device
	QString host = params["host"].toString("");
	if ( !host.isEmpty() )
	{
		QString username = params["user"].toString("");
		int lightId = params["lightId"].toInt(0);

		// Resolve hostname and port (or use default API port)
		#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
				QStringList addressparts = host.split(":", Qt::SkipEmptyParts);
		#else
				QStringList addressparts = host.split(":", QString::SkipEmptyParts);
		#endif

		QString apiHost = addressparts[0];
		int apiPort;

		if ( addressparts.size() > 1)
			apiPort = addressparts[1].toInt();
		else
			apiPort   = API_DEFAULT_PORT;

		initRestAPI(apiHost, apiPort, username);

		QString resource = QString("%1/%2/%3").arg( API_LIGHTS ).arg( lightId ).arg( API_STATE);
		_restApi->setPath(resource);

		QString stateCmd;
		stateCmd += QString("\"%1\":%2,").arg( API_STATE_ON, API_STATE_VALUE_TRUE );
		stateCmd += QString("\"%1\":\"%2\"").arg( "alert", "select" );
		stateCmd = "{" + stateCmd + "}";

		// Perform request
		httpResponse response = _restApi->put(stateCmd);
		if ( response.error() )
		{
			Warning (_log, "%s identification failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		}
	}
}
