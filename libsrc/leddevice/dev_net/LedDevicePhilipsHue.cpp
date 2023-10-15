// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

#include <chrono>

#include <utils/QStringUtils.h>
#include "qendian.h"

#include <ssdp/SSDPDiscover.h>

// mDNS discover
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#endif
#include <utils/NetUtils.h>

// Constants
namespace {

bool verbose = false;
bool verbose3 = false;

// Configuration settings
const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";
const char CONFIG_USERNAME[] = "username";
const char CONFIG_CLIENTKEY[] = "clientkey";
const char CONFIG_BRIGHTNESSFACTOR[] = "brightnessFactor";
const char CONFIG_TRANSITIONTIME[] = "transitiontime";
const char CONFIG_BLACK_LIGHTS_TIMEOUT[] = "blackLightsTimeout";
const char CONFIG_ON_OFF_BLACK[] = "switchOffOnBlack";
const char CONFIG_RESTORE_STATE[] = "restoreOriginalState";
const char CONFIG_lightIdS[] = "lightIds";
const char CONFIG_USE_HUE_API_V2[] = "useAPIv2";
const char CONFIG_USE_HUE_ENTERTAINMENT_API[] = "useEntertainmentAPI";
const char CONFIG_groupId[] = "groupId";

const char CONFIG_VERBOSE[] = "verbose";

// Philips Hue OpenAPI URLs
const int API_DEFAULT_PORT = -1; //Use default port per communication scheme
const char API_ROOT[] = "/";
const char API_BASE_PATH_V1[] = "api";
const char API_BASE_PATH_V2[] = "/clip/v2/resource";
const char API_AUTH_PATH_V1[] = "auth/v1";
const char API_RESOURCE_CONFIG[] = "config";
const char API_RESOURCE_LIGHTS[] = "lights";
const char API_RESOURCE_GROUPS[] = "groups";
//V2
const char API_RESOURCE_DEVICE[] = "device";
const char API_RESOURCE_LIGHT[] = "light";
const char API_RESOURCE_ENTERTAINMENT[] = "entertainment";
const char API_RESOURCE_ENTERTAINMENT_CONFIGURATION[] = "entertainment_configuration";

// Device Data elements
const char DEV_DATA_BRIDGEID[] = "bridgeid";
const char DEV_DATA_SOFTWAREVERSION[] = "swversion";
const char DEV_DATA_APIVERSION[] = "apiversion";

const char DEV_DATA_METADATA[] = "metadata";
const char DEV_DATA_NAME[] = "name";
const char DEV_DATA_ARCHETYPE[] = "archetype";

const char DEV_DATA_PRODUCTDATA[] = "product_data";
const char DEV_DATA_PRODUCT[] = "product_name";
const char DEV_DATA_MODEL[] = "model_id";

const char DEV_DATA_PRODUCT_V1[] = "productname";
const char DEV_DATA_MODEL_V1[] = "modelid";

// List of Group / Stream Information
const char API_GROUP_NAME[] = "name";
const char API_GROUP_TYPE[] = "type";
const char API_GROUP_TYPE_ENTERTAINMENT_V1[] = "Entertainment";
const char API_GROUP_TYPE_ENTERTAINMENT_CONFIGURATION[] = "entertainment_configuration";

const char API_OWNER[] = "owner";
const char API_STREAM[] = "stream";
const char API_STREAM_ACTIVE[] = "active";
const char API_STREAM_ACTIVE_VALUE_TRUE[] = "true";
const char API_STREAM_ACTIVE_VALUE_FALSE[] = "false";
const char API_STREAM_RESPONSE_FORMAT[] = "/%1/%2/%3/%4";
const char API_STREAM_STATUS[] = "status";
const char API_STREAM_ACTIVE_V2[] = "active_streamer";

const char API_LIGHT_SERVICES[] = "light_services";
const char API_CHANNELS[] = "channels";
const char API_RID[] = "rid";

// List of light resources
const char API_LIGTH_ID[] = "lightId";
const char API_LIGTH_ID_v1[] = "lightId_v1";
const char API_COLOR[] = "color";
const char API_GRADIENT[] = "gradient";
const char API_XY_COORDINATES[] = "xy";
const char API_X_COORDINATE[] = "x";
const char API_Y_COORDINATE[] = "y";
const char API_BRIGHTNESS[] = "bri";
const char API_TRANSITIONTIME[] = "transitiontime";

const char API_DYNAMICS[] = "dynamics";
const char API_DURATION[] = "duration";
const char API_DIMMING[] = "dimming";

// List of State Information
const char API_STATE[] = "state";
const char API_STATE_ON[] = "on";

// List of Action Information
const char API_ACTION[] = "action";
const char API_ACTION_START[] = "start";
const char API_ACTION_STOP[] = "stop";
const char API_ACTION_BREATHE[] = "breathe";

const char API_ALERT[] = "alert";
const char API_SELECT[] = "select";

// List of Data/Error Information
const char API_DATA[] = "data";
const char API_ERROR[] = "error";
const char API_ERROR_ADDRESS[] = "address";
const char API_ERROR_DESCRIPTION[] = "description";
const char API_ERROR_TYPE[] = "type";

const char API_ERRORS[] = "errors";

// List of Success Information
const char API_SUCCESS[] = "success";

// List of custom HTTP Headers
const char HTTP_HEADER_APPLICATION_KEY[] = "hue-application-key";

// Phlips Hue ssdp services
const char SSDP_ID[] = "upnp:rootdevice";
const char SSDP_FILTER[] = "(.*)IpBridge(.*)";
const char SSDP_FILTER_HEADER[] = "SERVER";

// DTLS Connection / SSL / Cipher Suite
const char API_SSL_SERVER_NAME[] = "Hue";
const char API_SSL_SEED_CUSTOM[] = "dtls_client";
const int API_SSL_SERVER_PORT = 2100;
const char API_SSL_CA_CERTIFICATE_RESSOURCE[] = ":/philips_hue_ca.pem";

const int STREAM_CONNECTION_RETRYS = 20;
const int STREAM_SSL_HANDSHAKE_ATTEMPTS = 5;
const int SSL_CIPHERSUITES[2] = { MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256, 0 };

const int DEV_FIRMWAREVERSION_APIV2 = 1948086000;

//Enable rewrites that Hue-Bridge does not close the connection ("After 10 seconds of no activity the connection is closed automatically, and status is set back to inactive.")
constexpr std::chrono::milliseconds STREAM_REWRITE_TIME{5000};

//Streaming message header and payload definition
const uint8_t HEADER[] =
{
	'H', 'u', 'e', 'S', 't', 'r', 'e', 'a', 'm', //protocol
	0x01, 0x00, //version 1.0
	0x01, //sequence number 1
	0x00, 0x00, //Reserved write 0’s
	0x00, // 0x00 = RGB; 0x01 = XY Brightness
	0x00, // Reserved, write 0’s
};

const uint8_t PAYLOAD_PER_LIGHT[] =
{
	0x01, 0x00, 0x06, //light ID
	//color: 16 bpc
	0xff, 0xff, //Red
	0xff, 0xff, //Green
	0xff, 0xff, //Blue
};

//API v2 - Streaming message header and payload definition
const uint8_t HEADER_V2[] =
{
	'H', 'u', 'e', 'S', 't', 'r', 'e', 'a', 'm', //protocol
	0x02, 0x00, //version 2.0
	0x01, //sequence number 1
	0x00, 0x00, //Reserved write 0’s
	0x00, // 0x00 = RGB; 0x01 = XY Brightness
	0x00, // Reserved
};

const char* ENTERTAINMENT_ID[36];
const uint8_t PAYLOAD_PER_CHANNEL_V2[] =
{
	0xff, //channel id
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff //color
};

} //End of constants

bool operator ==(const CiColor& p1, const CiColor& p2)
{
	return ((p1.x == p2.x) && (p1.y == p2.y) && (p1.bri == p2.bri));
}

bool operator != (const CiColor& p1, const CiColor& p2)
{
	return !(p1 == p2);
}

CiColor CiColor::rgbToCiColor(double red, double green, double blue, const CiColorTriangle& colorSpace, bool candyGamma)
{
	double cx;
	double cy;
	double bri;

	if (red + green + blue > 0)
	{
		// Apply gamma correction.
		double r = red;
		double g = green;
		double b = blue;

		if (candyGamma)
		{
			r = (red > 0.04045) ? pow((red + 0.055) / (1.0 + 0.055), 2.4) : (red / 12.92);
			g = (green > 0.04045) ? pow((green + 0.055) / (1.0 + 0.055), 2.4) : (green / 12.92);
			b = (blue > 0.04045) ? pow((blue + 0.055) / (1.0 + 0.055), 2.4) : (blue / 12.92);
		}

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

	if( (red + green + blue) > 0)
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
	: ProviderUdpSSL(deviceConfig)
	, _restApi(nullptr)
	, _apiPort(API_DEFAULT_PORT)
	, _useEntertainmentAPI(false)
	, _isAPIv2Ready(false)
	, _isDiyHue(false)
	, _api_major(0)
	, _api_minor(0)
	, _api_patch(0)
	, _isHueEntertainmentReady(false)
{
#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
							  Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType(_activeDeviceType)));
#endif
}

LedDevicePhilipsHueBridge::~LedDevicePhilipsHueBridge()
{
	delete _restApi;
	_restApi = nullptr;
}

bool LedDevicePhilipsHueBridge::init(const QJsonObject &deviceConfig)
{
	DebugIf( verbose, _log, "deviceConfig: [%s]", QJsonDocument(_devConfig).toJson(QJsonDocument::Compact).constData() );

	bool isInitOK = false;

	//Set hostname as per configuration and default port
	_hostName = deviceConfig[CONFIG_HOST].toString();
	_apiPort = deviceConfig[CONFIG_PORT].toInt();
	_authToken = deviceConfig[CONFIG_USERNAME].toString();

	Debug(_log, "Hostname/IP: %s", QSTRING_CSTR(_hostName) );

	_useApiV2 = deviceConfig[CONFIG_USE_HUE_API_V2].toBool(false);
	Debug(_log, "Use Hue API v2: %s", _useApiV2 ? "Yes" : "No" );

	if( _useEntertainmentAPI )
	{
		setLatchTime( 0);
		_devConfig["sslport"]        = API_SSL_SERVER_PORT;
		_devConfig["servername"]     = API_SSL_SERVER_NAME;
		_devConfig["psk"]            = _devConfig[ CONFIG_CLIENTKEY ].toString();
		if (_useApiV2)
		{
			// psk_identity is to be set later when application-id was resolved
			_devConfig["psk_identity"] = "";
		}
		else
		{
			_devConfig["psk_identity"]   = _authToken;
		}
		_devConfig["seed_custom"]    = API_SSL_SEED_CUSTOM;
		_devConfig["retry_left"]     = STREAM_CONNECTION_RETRYS;
		_devConfig["hs_attempts"]    = STREAM_SSL_HANDSHAKE_ATTEMPTS;
		_devConfig["hs_timeout_min"] = 600;
		_devConfig["hs_timeout_max"] = 1000;

		_port      = API_SSL_SERVER_PORT;

		isInitOK = ProviderUdpSSL::init(_devConfig);
	}
	else
	{
		isInitOK = LedDevice::init(_devConfig); // NOLINT
	}

	return isInitOK;
}

bool LedDevicePhilipsHueBridge::openRestAPI()
{
	bool isInitOK {true};

	if (_restApi == nullptr)
	{
		_restApi = new ProviderRestApi(_address.toString(), _apiPort);
		_restApi->setLogger(_log);
	}
	else
	{
		_restApi->setHost(_address.toString());
		_restApi->setPort(_apiPort);
	}

	if (_apiPort == 0 || _apiPort == 80 || _apiPort == 443)
	{
		_apiPort = API_DEFAULT_PORT;
		_restApi->setPort(_apiPort);
	}

	_restApi->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::checkApiError(const QJsonDocument &response, bool supressError)
{
	bool apiError = false;
	QString errorReason;

	DebugIf(verbose, _log, "Reply: [%s]", response.toJson(QJsonDocument::Compact).constData());

	if (_useApiV2)
	{
		QJsonObject obj = response.object();
		if (obj.contains(API_ERRORS))
		{
			const QJsonArray errorList = obj.value(API_ERRORS).toArray();
			if (!errorList.isEmpty())
			{
				QStringList errors;
				for (const QJsonValue &error : errorList)
				{
					QString errorString = error.toObject()[API_ERROR_DESCRIPTION].toString();
					if (!errorString.contains("may not have effect"))
					{
						errors << errorString;
					}
				}
				if (!errors.isEmpty())
				{
					errorReason = errors.join(",");
					apiError = true;
				}
			}
		}
	}
	else
	{
		QJsonArray responseList = response.array();
		if (!responseList.isEmpty())
		{
			QJsonObject respose = responseList.first().toObject();
			if (respose.contains(API_ERROR))
			{
				QJsonObject error = respose.value(API_ERROR).toObject();
				int errorType = error.value(API_ERROR_TYPE).toInt();
				QString errorDesc = error.value(API_ERROR_DESCRIPTION).toString();
				QString errorAddress = error.value(API_ERROR_ADDRESS).toString();

				if( errorType != 901 )
				{
					errorReason = QString ("(%1) %2, Resource:%3").arg(errorType).arg(errorDesc, errorAddress);
					apiError = true;
				}
			}
		}
	}

	if (apiError)
	{
		if (!supressError)
		{
			this->setInError(errorReason);
		}
		else
		{
			Warning(_log, "Suppresing error: %s", QSTRING_CSTR(errorReason));
		}
	}
	return apiError;
}

int LedDevicePhilipsHueBridge::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	{
		if ( openRestAPI() )
		{
			QJsonDocument bridgeDetails = retrieveBridgeDetails();
			if ( !bridgeDetails.isEmpty() )
			{
				setBridgeDetails(bridgeDetails, true);


				if (_useApiV2)
				{
					if ( configureSsl() )
					{
						if (retrieveApplicationId())
						{
							setPSKidentity(_applicationID);
						}
					}
				}
				else
				{
					if (_isAPIv2Ready)
					{
						Warning(_log,"Your Hue Bridge supports a newer API. Reconfigure your device in Hyperion to benefit from new features.");
					}
				}

				if (!isInError() )
				{
					setBaseApiEnvironment(_useApiV2);
					if (initLightsMap() && initDevicesMap() && initEntertainmentSrvsMap())
					{
						if ( _useEntertainmentAPI )
						{
							if (initGroupsMap())
							{
								// Open bridge for streaming
								if ( ProviderUdpSSL::open() == 0 )
								{
									// Everything is OK, device is ready
									_isDeviceReady = true;
									retval = 0;
								}
							}
						}
						else
						{
							// Everything is OK, device is ready
							_isDeviceReady = true;
							retval = 0;
						}
					}
				}
			}
		}
	}

	return retval;
}

int LedDevicePhilipsHueBridge::close()
{
	_isDeviceReady = false;
	int retval = 0;

	if( _useEntertainmentAPI )
	{
		retval = ProviderUdpSSL::close();
	}

	return retval;
}

bool LedDevicePhilipsHueBridge::configureSsl()
{
	_restApi->setAlternateServerIdentity(_deviceBridgeId);

	if (_isDiyHue)
	{
		_restApi->acceptSelfSignedCertificates(true);
	}

	bool success = _restApi->setCaCertificate(API_SSL_CA_CERTIFICATE_RESSOURCE);
	if (!success)
	{
		this->setInError ( "Failed to configure Hue Bridge for SSL", false );
	}

	return success;
}

const int *LedDevicePhilipsHueBridge::getCiphersuites() const
{
	return SSL_CIPHERSUITES;
}

void LedDevicePhilipsHueBridge::log(const char* msg, const char* type, ...) const
{
	const size_t max_val_length = 1024;
	char val[max_val_length];
	va_list args;
	va_start(args, type);
	vsnprintf(val, max_val_length, type, args);
	va_end(args);
	std::string s = msg;
	size_t max = 30;
	if (max > s.length())
	{
		s.append(max - s.length(), ' ');
	}
	Debug( _log, "%s: %s", s.c_str(), val );
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveBridgeDetails()
{
	QJsonDocument bridgeDetails;
	if ( openRestAPI() )
	{
		setBaseApiEnvironment(false, API_BASE_PATH_V1);
		bridgeDetails = get( API_RESOURCE_CONFIG );
	}
	return bridgeDetails;
}

bool LedDevicePhilipsHueBridge::retrieveApplicationId()
{
	bool rc {false};

	setBaseApiEnvironment(true, API_ROOT);
	_restApi->setPath(API_AUTH_PATH_V1);

	httpResponse response = _restApi->get();

	if ( !response.error() )
	{
		_applicationID = response.getHeader("hue-application-id");
		rc = true;
	}
	else
	{
		QString errorReason = QString("Failed to get application-id from Hue Bridge, error: '%1'").arg(response.getErrorReason());
		this->setInError ( errorReason, false );
	}
	return rc;
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveDeviceDetails(const QString& deviceId )
{
	QStringList resourcePath;
	if (_useApiV2)
	{
		resourcePath << API_RESOURCE_DEVICE;
		if (!deviceId.isEmpty())
		{
			resourcePath << deviceId;
		}
	}
	return get( resourcePath );
}


QJsonDocument LedDevicePhilipsHueBridge::retrieveLightDetails(const QString& lightId )
{
	QStringList resourcePath;
	if (_useApiV2)
	{
		resourcePath << API_RESOURCE_LIGHT;
		if (!lightId.isEmpty())
		{
			resourcePath << lightId;
		}
	}
	else
	{
		resourcePath << API_RESOURCE_LIGHTS;
		if (!lightId.isEmpty())
		{
			resourcePath << lightId << API_STATE;
		}
	}
	return get( resourcePath );
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveGroupDetails(const QString& groupId )
{
	QStringList resourcePath;
	if (_useApiV2)
	{
		resourcePath << API_RESOURCE_ENTERTAINMENT_CONFIGURATION;
		if (!groupId.isEmpty())
		{
			resourcePath << groupId;
		}
	}
	else
	{
		resourcePath << API_RESOURCE_GROUPS;
		if (!groupId.isEmpty())
		{
			resourcePath << groupId;
		}
	}
	return get( resourcePath );
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveEntertainmentSrvDetails(const QString& entertainmentID )
{
	QStringList resourcePath;
	if (_useApiV2)
	{
		resourcePath << API_RESOURCE_ENTERTAINMENT;
		if (!entertainmentID.isEmpty())
		{
			resourcePath << entertainmentID;
		}
	}
	return get( resourcePath );
}


bool LedDevicePhilipsHueBridge::isApiEntertainmentReady(const QString& apiVersion)
{
	bool ready {false};

	QStringList apiVersionParts = QStringUtils::split(apiVersion,".", QStringUtils::SplitBehavior::SkipEmptyParts);
	if ( !apiVersionParts.isEmpty() )
	{
		_api_major = apiVersionParts[0].toUInt();
		_api_minor = apiVersionParts[1].toUInt();
		_api_patch = apiVersionParts[2].toUInt();

		if ( _api_major > 1 || (_api_major == 1 && _api_minor >= 22) )
		{
			ready = true;
		}
	}
	Debug(_log,"API version [%s] %s Entertainment API ready", QSTRING_CSTR(apiVersion), ready ? "is" : "is not" );
	return ready;
}

bool LedDevicePhilipsHueBridge::isAPIv2Ready(int swVersion)
{
	bool ready {true};
	if (swVersion < DEV_FIRMWAREVERSION_APIV2)
	{
		ready = false;
	}
	Debug(_log,"Firmware version [%d] %s API v2 ready", swVersion, ready ? "is" : "is not" );
	return ready;
}

void LedDevicePhilipsHueBridge::setBaseApiEnvironment(bool apiV2, const QString& path)
{
	if ( _restApi != nullptr )
	{
		QStringList basePath;
		if (apiV2)
		{
			_restApi->setScheme("https");

			if (!path.isEmpty())
			{
				basePath << path;
			}
			else
			{
				basePath << API_BASE_PATH_V2;
			}
			_restApi->setHeader(HTTP_HEADER_APPLICATION_KEY, _authToken.toUtf8());
		}
		else
		{
			_restApi->setScheme("http");

			if (!path.isEmpty())
			{
				basePath << path;
			}
			else
			{
				//Base-path is api-path + authentication token (here username)
				basePath << API_BASE_PATH_V1 << _authToken;
			}
		}
		_restApi->setBasePath(basePath);

		DebugIf(verbose, _log,"New BasePath: %s", QSTRING_CSTR(_restApi->getBasePath()));
	}
}

bool LedDevicePhilipsHueBridge::initDevicesMap()
{
	bool isInitOK = false;

	if ( !this->isInError() )
	{
		QJsonDocument deviceDetails = retrieveDeviceDetails();
		if ( !deviceDetails.isEmpty() )
		{
			setDevicesMap( deviceDetails );
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::initLightsMap()
{
	bool isInitOK = false;

	if ( !this->isInError() )
	{
		QJsonDocument lightDetails = retrieveLightDetails();
		if ( !lightDetails.isEmpty() )
		{
			setLightsMap( lightDetails );
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::initGroupsMap()
{
	bool isInitOK = false;

	if ( !this->isInError() )
	{
		QJsonDocument groupDetails = retrieveGroupDetails();
		if ( !groupDetails.isEmpty() )
		{
			setGroupMap( groupDetails );
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::initEntertainmentSrvsMap()
{
	bool isInitOK = false;

	if ( !this->isInError() )
	{
		QJsonDocument entertainmentSrvDetails = retrieveEntertainmentSrvDetails();
		if ( !entertainmentSrvDetails.isEmpty() )
		{
			setEntertainmentSrvMap( entertainmentSrvDetails );
			isInitOK = true;
		}
	}
	return isInitOK;
}

void LedDevicePhilipsHueBridge::setBridgeDetails(const QJsonDocument &doc, bool isLogging)
{
	QJsonObject jsonConfigInfo = doc.object();
	if ( verbose )
	{
		std::cout <<  "jsonConfigInfo: [" << QJsonDocument(jsonConfigInfo).toJson(QJsonDocument::Compact).constData() << "]" << std::endl;
	}

	_deviceName = jsonConfigInfo[DEV_DATA_NAME].toString();
	if (_deviceName.startsWith("DiyHue", Qt::CaseInsensitive))
	{
		_isDiyHue = true;
	}

	_deviceModel = jsonConfigInfo[DEV_DATA_MODEL_V1].toString();
	_deviceBridgeId = jsonConfigInfo[DEV_DATA_BRIDGEID].toString();
	_deviceFirmwareVersion = jsonConfigInfo[DEV_DATA_SOFTWAREVERSION].toString().toInt();
	_deviceAPIVersion = jsonConfigInfo[DEV_DATA_APIVERSION].toString();

	_isHueEntertainmentReady = isApiEntertainmentReady(_deviceAPIVersion);
	_isAPIv2Ready = isAPIv2Ready(_deviceFirmwareVersion);

	if( _useEntertainmentAPI )
	{
		DebugIf( !_isHueEntertainmentReady, _log, "Bridge is not Entertainment API Ready - Entertainment API usage was disabled!" );
		_useEntertainmentAPI = _isHueEntertainmentReady;
	}

	if (isLogging)
	{
		log( "Bridge Name", "%s", QSTRING_CSTR( _deviceName ));
		log( "Bridge-ID", "%s", QSTRING_CSTR( _deviceBridgeId ));
		log( "Model", "%s", QSTRING_CSTR( _deviceModel ));
		log( "Firmware version", "%d", _deviceFirmwareVersion );
		log( "API-Version", "%u.%u.%u", _api_major, _api_minor, _api_patch );
		log( "API v2 ready", "%s", _isAPIv2Ready ? "Yes" : "No" );
		log( "Entertainment ready", "%s", _isHueEntertainmentReady ? "Yes" : "No" );
		log( "DIYHue", "%s", _isDiyHue ? "Yes" : "No" );
	}
}

void LedDevicePhilipsHueBridge::setDevicesMap(const QJsonDocument &doc)
{
	_devicesMap.clear();

	if (_useApiV2)
	{
		const QJsonArray devices = doc.array();

		for (const QJsonValue &device : devices)
		{
			QString deviceId = device.toObject().value("id").toString();
			_devicesMap.insert(deviceId, device.toObject());
		}
	}
}

void LedDevicePhilipsHueBridge::setLightsMap(const QJsonDocument &doc)
{
	_lightsMap.clear();

	if (_useApiV2)
	{
		const QJsonArray lights = doc.array();

		for (const QJsonValue &light : lights)
		{
			QString lightId = light.toObject().value("id").toString();
			_lightsMap.insert(lightId, light.toObject());
		}
	}
	else
	{
		QJsonObject jsonLightsInfo = doc.object();
		DebugIf(verbose, _log, "jsonLightsInfo: [%s]", QJsonDocument(jsonLightsInfo).toJson(QJsonDocument::Compact).constData() );

		// Get all available light ids and their values
		QStringList keys = jsonLightsInfo.keys();

		for ( int i = 0; i < keys.count(); ++i )
		{
			QString key = keys.at(i);
			_lightsMap.insert(key, jsonLightsInfo[key].toObject());
		}
	}

	_lightsCount = _lightsMap.count();

	if ( _lightsCount == 0 )
	{
		this->setInError( "No light-IDs found at the Philips Hue Bridge" );
	}
	else
	{
		log( "Lights at Bridge found", "%d", _lightsCount );
	}
}

void LedDevicePhilipsHueBridge::setGroupMap(const QJsonDocument &doc)
{
	_groupsMap.clear();
	if (_useApiV2)
	{
		const QJsonArray groups = doc.array();

		for (const QJsonValue &group : groups)
		{
			QString groupId = group.toObject().value("id").toString();
			_groupsMap.insert(groupId, group.toObject());
		}
	}
	else
	{
		QJsonObject jsonGroupsInfo = doc.object();
		DebugIf(verbose, _log, "jsonGroupsInfo: [%s]", QJsonDocument(jsonGroupsInfo).toJson(QJsonDocument::Compact).constData() );

		// Get all available group ids and their values
		QStringList keys = jsonGroupsInfo.keys();

		int _groupsCount = keys.size();
		for ( int i = 0; i < _groupsCount; ++i )
		{
			_groupsMap.insert( keys.at(i), jsonGroupsInfo.take(keys.at(i)).toObject() );
		}
	}
}

void LedDevicePhilipsHueBridge::setEntertainmentSrvMap(const QJsonDocument &doc)
{
	_entertainmentMap.clear();

	if (_useApiV2)
	{
		const QJsonArray entertainmentSrvs = doc.array();

		for (const QJsonValue &entertainmentSrv : entertainmentSrvs)
		{
			QString entertainmentSrvId = entertainmentSrv.toObject().value("id").toString();
			_entertainmentMap.insert(entertainmentSrvId, entertainmentSrv.toObject());
		}
	}
}

QMap<QString,QJsonObject> LedDevicePhilipsHueBridge::getDevicesMap() const
{
	return _devicesMap;
}

QMap<QString,QJsonObject> LedDevicePhilipsHueBridge::getLightMap() const
{
	return _lightsMap;
}

QMap<QString,QJsonObject> LedDevicePhilipsHueBridge::getGroupMap() const
{
	return _groupsMap;
}

QMap<QString,QJsonObject> LedDevicePhilipsHueBridge::getEntertainmentMap() const
{
	return _entertainmentMap;
}

QJsonObject LedDevicePhilipsHueBridge::getDeviceDetails(const QString& deviceId)
{
	DebugIf( verbose, _log, "[%s]", QSTRING_CSTR(deviceId) );
	return _devicesMap.value(deviceId);
}

QJsonObject LedDevicePhilipsHueBridge::getLightDetails(const QString& lightId)
{
	DebugIf( verbose, _log, "[%s]", QSTRING_CSTR(lightId) );
	return _lightsMap.value(lightId);
}

QJsonDocument LedDevicePhilipsHueBridge::setLightState(const QString& lightId, const QJsonObject& state)
{
	DebugIf( verbose, _log, "[%s] ", QSTRING_CSTR(lightId) );
	QStringList resourcePath;
	QJsonObject cmd;

	if (_useApiV2)
	{
		resourcePath << API_RESOURCE_LIGHT << lightId;
		cmd = state;
	}
	else
	{
		resourcePath << API_RESOURCE_LIGHTS << lightId << API_STATE;
		cmd = state;
	}
	return put(resourcePath, cmd);
}

QJsonDocument LedDevicePhilipsHueBridge::getGroupDetails(const QString& groupId)
{
	DebugIf( verbose, _log, "[%s]", QSTRING_CSTR(groupId) );
	return retrieveGroupDetails(groupId);
}

QString LedDevicePhilipsHueBridge::getGroupName(const QString& groupId ) const
{
	QString groupName;
	if( _groupsMap.contains( groupId ) )
	{
		QJsonObject group = _groupsMap.value( groupId );
		groupName = group.value( API_GROUP_NAME ).toString().trimmed().replace("\"", "");
		DebugIf( verbose, _log, "GroupId [%s]: GroupName: %s", QSTRING_CSTR(groupId), QSTRING_CSTR(groupName) );
	}
	else
	{
		Error(_log, "Group ID %s does not exist on this bridge", QSTRING_CSTR(groupId) );
	}
	return groupName;
}

QStringList LedDevicePhilipsHueBridge::getGroupLights(const QString& groupId) const
{
	QStringList groupLights;
	// search user groupId inside _groupsMap and create light if found
	if( _groupsMap.contains( groupId ) )
	{
		QJsonObject group = _groupsMap.value( groupId );

		QString groupName = getGroupName( groupId );

		QString type = group.value( API_GROUP_TYPE ).toString();
		if( type == API_GROUP_TYPE_ENTERTAINMENT_V1 || type ==  API_GROUP_TYPE_ENTERTAINMENT_CONFIGURATION)
		{
			if (_useApiV2)
			{
				const QJsonArray lightServices = group.value( API_LIGHT_SERVICES ).toArray();
				for (const QJsonValue &light : lightServices)
				{
					groupLights.append( light.toObject().value(API_RID).toString());
				}
			}
			else
			{
				groupLights = group.value( API_RESOURCE_LIGHTS ).toVariant().toStringList();
			}
			Info(_log, "Entertainment Group \"%s\" [%s] with %d lights found", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId), groupLights.size() );
		}
		else
		{
			Error(_log, "Group ID (%s)[%s] is not an entertainment group", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId));
		}
	}
	else
	{
		Error(_log, "Group ID [%s] does not exist on this bridge", QSTRING_CSTR(groupId) );
	}
	return groupLights;
}

QJsonDocument LedDevicePhilipsHueBridge::setGroupState(const QString&  groupId, bool state)
{
	QStringList resourcePath;
	QJsonObject cmd;

	if (_useApiV2)
	{
		resourcePath << API_RESOURCE_ENTERTAINMENT_CONFIGURATION << groupId;
		cmd.insert(API_ACTION, state ? API_ACTION_START : API_ACTION_STOP);
	}
	else
	{
		resourcePath << API_RESOURCE_GROUPS << groupId;
		cmd.insert(API_STREAM, QJsonObject {{API_STREAM_ACTIVE, state }});
	}
	return put(resourcePath, cmd);
}

QJsonObject LedDevicePhilipsHueBridge::getEntertainmentSrvDetails(const QString& deviceId)
{
	DebugIf( verbose, _log, "getEntertainmentSrvDetails [%s]", QSTRING_CSTR(deviceId) );

	QJsonObject details;
	for (const QJsonObject& entertainmentSrv : std::as_const(_entertainmentMap))
	{
		QJsonObject owner = entertainmentSrv[API_OWNER].toObject();

		if (owner[API_RID] == deviceId)
		{
			details = entertainmentSrv;
			break;
		}
	}
	return details;
}

int LedDevicePhilipsHueBridge::getGroupChannelsCount(const QString& groupId) const
{
	int channelsCount {0};
	// search user groupId inside _groupsMap and create light if found
	if( _groupsMap.contains( groupId ) )
	{
		QJsonObject group = _groupsMap.value( groupId );
		QString groupName = getGroupName( groupId );
		QString type = group.value( API_GROUP_TYPE ).toString();
		if(type ==  API_GROUP_TYPE_ENTERTAINMENT_CONFIGURATION)
		{
			if (_useApiV2)
			{
				QJsonArray channels = group.value( API_CHANNELS ).toArray();
				channelsCount = channels.size();
			}
			Info(_log, "Entertainment Group \"%s\" [%s] with %d channels found", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId), channelsCount );
		}
		else
		{
			Error(_log, "Group ID (%s)[%s] is not an entertainment group", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId));
		}
	}
	else
	{
		Error(_log, "Group ID [%s] does not exist on this bridge", QSTRING_CSTR(groupId) );
	}
	return channelsCount;
}

QJsonDocument LedDevicePhilipsHueBridge::get(const QString& route)
{
	return get(QStringList{route});
}

QJsonDocument LedDevicePhilipsHueBridge::get(const QStringList& routeElements)
{
	_restApi->setPath(routeElements);
	httpResponse response = _restApi->get();
	if (response.error())
	{
		if (routeElements.isEmpty() &&
			(  response.getNetworkReplyError() == QNetworkReply::UnknownNetworkError ||
			   response.getNetworkReplyError() == QNetworkReply::ConnectionRefusedError ||
			   response.getNetworkReplyError() == QNetworkReply::RemoteHostClosedError ||
			   response.getNetworkReplyError() == QNetworkReply::OperationCanceledError ))
		{
			Warning(_log, "API request (Get): The Hue Bridge is not ready.");
		}
		else
		{
			QString errorReason = QString("API request (Get) failed with error: '%1'").arg(response.getErrorReason());
			this->setInError ( errorReason );
		}
	}
	else
	{
		if (!checkApiError(response.getBody()))
		{
			if (_useApiV2)
			{
				QJsonObject obj = response.getBody().object();
				if (obj.contains(API_DATA))
				{
					return QJsonDocument {obj.value(API_DATA).toArray()};
				}
			}
		}
	}
	_restApi->clearPath();
	return response.getBody();
}

QJsonDocument LedDevicePhilipsHueBridge::put(const QStringList& routeElements, const QJsonObject& content, bool supressError)
{
	_restApi->setPath(routeElements);
	httpResponse response = _restApi->put(content);
	if (response.error())
	{
		QString errorReason = QString("API request (Put) failed with error: '%1'").arg(response.getErrorReason());
		this->setInError ( errorReason );
	}
	else
	{
		if (!checkApiError(response.getBody(), supressError))
		{
			if (_useApiV2)
			{
				QJsonObject obj = response.getBody().object();
				if (obj.contains(API_DATA))
				{
					return QJsonDocument {obj.value(API_DATA).toArray()};
				}
			}
		}
	}
	_restApi->clearPath();
	return response.getBody();
}

bool LedDevicePhilipsHueBridge::isStreamOwner(const QString &streamOwner) const
{
	bool isOwner {false};

	if (_useApiV2)
	{
		if ( streamOwner != "" && streamOwner == _applicationID)
		{
			isOwner = true;
		}
	}
	else
	{
		if ( streamOwner != "" && streamOwner == _authToken )
		{
			isOwner = true;
		}
	}
	return isOwner;
}

QJsonArray LedDevicePhilipsHueBridge::discoverSsdp()
{
	QJsonArray deviceList;
	SSDPDiscover discover;
	discover.skipDuplicateKeys(true);
	discover.setSearchFilter(SSDP_FILTER, SSDP_FILTER_HEADER);
	QString searchTarget = SSDP_ID;

	if (discover.discoverServices(searchTarget) > 0)
	{
		deviceList = discover.getServicesDiscoveredJson();
	}
	return deviceList;
}

QJsonObject LedDevicePhilipsHueBridge::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

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

	Debug(_log, "devicesDiscovered: [%s]", QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact).constData() );

	return devicesDiscovered;
}

QJsonObject LedDevicePhilipsHueBridge::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QJsonDocument(params).toJson(QJsonDocument::Compact).constData());
	QJsonObject properties;

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort =  params[CONFIG_PORT].toInt();
	_authToken = params[CONFIG_USERNAME].toString("");

	Info(_log, "Get properties for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName) );

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	{
		QJsonDocument bridgeDetails = retrieveBridgeDetails();
		if ( !bridgeDetails.isEmpty() )
		{
			setBridgeDetails(bridgeDetails);

			if ( openRestAPI() )
			{
				_useApiV2 = _isAPIv2Ready;
				if (_authToken == API_RESOURCE_CONFIG)
				{
					properties.insert("properties", bridgeDetails.object());
					properties.insert("isEntertainmentReady",_isHueEntertainmentReady);
					properties.insert("isAPIv2Ready",_isAPIv2Ready);
				}
				else
				{
					if (_useApiV2)
					{
						configureSsl();
					}

					if (!isInError() )
					{
						setBaseApiEnvironment(_useApiV2);

						QString filter = params["filter"].toString("");
						_restApi->setPath(filter);

						// Perform request
						httpResponse response = _restApi->get();
						if (response.error())
						{
							Warning(_log, "%s get properties failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
						}
						properties.insert("properties", response.getBody().object());
						properties.insert("isEntertainmentReady",_isHueEntertainmentReady);
						properties.insert("isAPIv2Ready",_isAPIv2Ready);
					}
				}
			}
		}
		DebugIf(verbose, _log, "properties: [%s]", QJsonDocument(properties).toJson(QJsonDocument::Compact).constData());
	}
	return properties;
}

QJsonObject LedDevicePhilipsHueBridge::addAuthorization(const QJsonObject& params)
{
	Debug(_log, "params: [%s]", QJsonDocument(params).toJson(QJsonDocument::Compact).constData());
	QJsonObject responseBody;

	// New Phillips-Bridge device client/application key
	_hostName = params[CONFIG_HOST].toString("");
	_apiPort =  params[CONFIG_PORT].toInt();

	Info(_log, "Add authorized user for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName) );

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	{
		QJsonDocument bridgeDetails = retrieveBridgeDetails();
		if ( !bridgeDetails.isEmpty() )
		{
			setBridgeDetails(bridgeDetails);
			if ( openRestAPI() )
			{
				_useApiV2 = _isAPIv2Ready;
				if (_useApiV2)
				{
					configureSsl();
				}

				if (!isInError() )
				{
					setBaseApiEnvironment(_useApiV2, API_BASE_PATH_V1);

					QJsonObject clientKeyCmd{ {"devicetype", "hyperion#" + QHostInfo::localHostName()}, {"generateclientkey", true } };
					_restApi->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

					httpResponse response = _restApi->post(clientKeyCmd);
					if (response.error())
					{
						Warning(_log, "%s generation of authorization/client key failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
					}
					else
					{
						if (!checkApiError(response.getBody(),false))
						{
							responseBody = response.getBody().array().first().toObject().value("success").toObject();
						}
					}
				}
			}
		}
	}
	return responseBody;
}

const std::set<QString> PhilipsHueLight::GAMUT_A_MODEL_IDS =
{ "LLC001", "LLC005", "LLC006", "LLC007", "LLC010", "LLC011", "LLC012", "LLC013", "LLC014", "LST001" };
const std::set<QString> PhilipsHueLight::GAMUT_B_MODEL_IDS =
{ "LCT001", "LCT002", "LCT003", "LCT007", "LLM001" };
const std::set<QString> PhilipsHueLight::GAMUT_C_MODEL_IDS =
{ "LCA001", "LCA002", "LCA003", "LCG002", "LCP001", "LCP002", "LCT010", "LCT011", "LCT012", "LCT014", "LCT015", "LCT016", "LCT024", "LCX001", "LCX002", "LLC020", "LST002" };

PhilipsHueLight::PhilipsHueLight(Logger* log, bool useApiV2, const QString& id, const QJsonObject& lightAttributes, int onBlackTimeToPowerOff,
								 int onBlackTimeToPowerOn)
	: _log(log)
	, _useApiV2(useApiV2)
	, _id(id)
	, _on(false)
	, _transitionTime(0)
	, _color({ 0.0, 0.0, 0.0 })
	, _hasColor(false)
	, _colorBlack({ 0.0, 0.0, 0.0 })
	, _lastSendColorTime(0)
	, _lastBlackTime(-1)
	, _lastWhiteTime(-1)
	, _blackScreenTriggered(false)
	, _onBlackTimeToPowerOff(onBlackTimeToPowerOff)
	, _onBlackTimeToPowerOn(onBlackTimeToPowerOn)
{
	if ( _useApiV2 )
	{
		QJsonObject lightOwner = lightAttributes[API_OWNER].toObject();
		_deviceId = lightOwner[API_RID].toString();
		_gamutType = lightAttributes[API_COLOR].toObject()["gamut_type"].toString();
	}
	else
	{
		_name = lightAttributes[DEV_DATA_NAME].toString().trimmed().replace("\"", "");
		_model = lightAttributes[DEV_DATA_MODEL_V1].toString();
		_product = lightAttributes[DEV_DATA_PRODUCT_V1].toString();

		// Find id in the sets and set the appropriate color space.
		if (GAMUT_A_MODEL_IDS.find(_model) != GAMUT_A_MODEL_IDS.end())
		{
			_gamutType = "A";
		}
		else if (GAMUT_B_MODEL_IDS.find(_model) != GAMUT_B_MODEL_IDS.end())
		{
			_gamutType = "B";
		}
		else if (GAMUT_C_MODEL_IDS.find(_model) != GAMUT_C_MODEL_IDS.end())
		{
			_gamutType = "C";
		}
		else
		{
			_gamutType = "";
		}
	}

	if (_gamutType.isEmpty())
	{
		Warning(_log, "Light \"%s\" [%s], did not recognize model [%s]", QSTRING_CSTR(_name), QSTRING_CSTR(id), QSTRING_CSTR(_model));
	}

	// Set the appropriate color space.
	if (_gamutType == "A")
	{
		_colorSpace.red		= {0.704, 0.296};
		_colorSpace.green	= {0.2151, 0.7106};
		_colorSpace.blue	= {0.138, 0.08};
		_colorBlack 		= {0.138, 0.08, 0.0};
	}
	else if (_gamutType == "B")
	{
		_colorSpace.red 	= {0.675, 0.322};
		_colorSpace.green	= {0.409, 0.518};
		_colorSpace.blue 	= {0.167, 0.04};
		_colorBlack 		= {0.167, 0.04, 0.0};
	}
	else if (_gamutType == "C")
	{
		_colorSpace.red		= {0.6915, 0.3083};
		_colorSpace.green	= {0.17, 0.7};
		_colorSpace.blue 	= {0.1532, 0.0475};
		_colorBlack 		= {0.1532, 0.0475, 0.0};
	}
	else
	{
		_colorSpace.red 	= {1.0, 0.0};
		_colorSpace.green 	= {0.0, 1.0};
		_colorSpace.blue 	= {0.0, 0.0};
		_colorBlack 		= {0.0, 0.0, 0.0};
	}
}

void PhilipsHueLight::setDeviceDetails(const QJsonObject& details)
{
	if (!details.isEmpty())
	{
		QJsonObject metaData = details[DEV_DATA_METADATA].toObject();
		_name = metaData[DEV_DATA_NAME].toString();
		_archeType = metaData[DEV_DATA_ARCHETYPE].toString();

		QJsonObject productData = details[DEV_DATA_PRODUCTDATA].toObject();
		_product = productData[DEV_DATA_PRODUCT].toString();
		_model = productData[DEV_DATA_MODEL].toString();
	}
}

void PhilipsHueLight::setEntertainmentSrvDetails(const QJsonObject& details)
{
	if (!details.isEmpty())
	{
		QJsonObject segmentData = details["segments"].toObject();
		_maxSegments = segmentData["max_segments"].toInt(1);
	}
	else
	{
		_maxSegments = 1;
	}
}

void PhilipsHueLight::blackScreenTriggered()
{
	_blackScreenTriggered = true;
}

bool PhilipsHueLight::isBusy()
{
	bool temp = true;

	qint64 _currentTime = QDateTime::currentMSecsSinceEpoch();
	if (_currentTime - _lastSendColorTime >= 100)
	{
		_lastSendColorTime = _currentTime;
		temp = false;
	}
	return temp;
}

void PhilipsHueLight::setBlack()
{
	Debug(_log,"");
	CiColor black;
	black.bri = 0;
	black.x = 0;
	black.y = 0;
	setColor(black);
}

bool PhilipsHueLight::isBlack(bool isBlack)
{
	if (!isBlack)
	{
		_lastBlackTime = 0;
		return false;
	}

	if (_lastBlackTime == 0)
	{
		_lastBlackTime = QDateTime::currentMSecsSinceEpoch();
		return false;
	}

	qint64 _currentTime = QDateTime::currentMSecsSinceEpoch();
	return _currentTime - _lastBlackTime >= _onBlackTimeToPowerOff;
}

bool PhilipsHueLight::isWhite(bool isWhite)
{
	if (!isWhite)
	{
		_lastWhiteTime = 0;
		return false;
	}

	if (_lastWhiteTime == 0)
	{
		_lastWhiteTime = QDateTime::currentMSecsSinceEpoch();
		return false;
	}

	qint64 _currentTime = QDateTime::currentMSecsSinceEpoch();
	return _currentTime - _lastWhiteTime >= _onBlackTimeToPowerOn;
}

QString PhilipsHueLight::getId() const
{
	return _id;
}

QString PhilipsHueLight::getdeviceId() const
{
	return _deviceId;
}

QString PhilipsHueLight::getProduct() const
{
	return _product;
}

QString PhilipsHueLight::getModel() const
{
	return _model;
}

QString PhilipsHueLight::getName() const
{
	return _name;
}

QString PhilipsHueLight::getArcheType() const
{
	return _archeType;
}

int PhilipsHueLight::getMaxSegments() const
{
	return _maxSegments;
}

QJsonObject PhilipsHueLight::getOriginalState() const
{
	return _originalState;
}

void PhilipsHueLight::saveOriginalState(const QJsonObject& values)
{
	Debug(_log,"Light: %s, id: %s", QSTRING_CSTR(_name), QSTRING_CSTR(_id));
	if ( _useApiV2 )
	{
		_originalState[API_STATE_ON] = values[API_STATE_ON];

		QJsonValue color = values[API_COLOR];
		_originalState.insert(API_COLOR, QJsonObject {{API_XY_COORDINATES, color[API_XY_COORDINATES] }});

		_originalState[API_GRADIENT] = values[API_GRADIENT];
	}
	else
	{
		if (_blackScreenTriggered)
		{
			_blackScreenTriggered = false;
			return;
		}
		// Get state object values which are subject to change.
		if (!values[API_STATE].toObject().contains("on"))
		{
			Error(_log, "Got invalid state object from light ID %s", QSTRING_CSTR(_id) );
		}
		QJsonObject lState = values[API_STATE].toObject();
		_originalStateJSON = lState;

		QJsonObject state;
		state["on"] = lState["on"];
		_originalColor = CiColor();
		_originalColor.bri = 0;
		_originalColor.x = 0;
		_originalColor.y = 0;
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
			Debug(_log, "Philips original state stored: %s", QSTRING_CSTR(c));
			_transitionTime = values[API_STATE].toObject()[API_TRANSITIONTIME].toInt();
		}
		//Determine the original state.
		_originalState = state;
	}
}

void PhilipsHueLight::setOnOffState(bool on)
{
	Debug(_log,"Light: %s, id: %s -> %s", QSTRING_CSTR(_name), QSTRING_CSTR(_id), on  ? "On" : "Off");
	this->_on = on;
}

void PhilipsHueLight::setTransitionTime(int transitionTime)
{
	this->_transitionTime = transitionTime;
}

void PhilipsHueLight::setColor(const CiColor& color)
{
	this->_hasColor = true;
	this->_color = color;
}

bool PhilipsHueLight::getOnOffState() const
{
	return _on;
}

int PhilipsHueLight::getTransitionTime() const
{
	return _transitionTime;
}

CiColor PhilipsHueLight::getColor() const
{
	return _color;
}

bool PhilipsHueLight::hasColor() const
{
	return _hasColor;
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
	, _isInitLeds(false)
	, _lightsCount(0)
	, _blackLightsTimeout(15000)
	, _blackLevel(0.0)
	, _onBlackTimeToPowerOff(100)
	, _onBlackTimeToPowerOn(100)
	, _candyGamma(true)
	, _groupStreamState(false)
{
}

LedDevice* LedDevicePhilipsHue::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePhilipsHue(deviceConfig);
}

LedDevicePhilipsHue::~LedDevicePhilipsHue()
{
}

bool LedDevicePhilipsHue::init(const QJsonObject &deviceConfig)
{
	bool isInitOK {false};

	if (!verbose)
	{
		verbose = deviceConfig[CONFIG_VERBOSE].toBool(false);
	}

	// Initialise LedDevice configuration and execution environment
	_useEntertainmentAPI = deviceConfig[CONFIG_USE_HUE_ENTERTAINMENT_API].toBool(false);

	// Overwrite non supported/required features
	if ( deviceConfig["rewriteTime"].toInt(0) > 0 )
	{
		InfoIf ( ( !_useEntertainmentAPI ), _log, "Device Philips Hue does not require rewrites. Refresh time is ignored." );
		_devConfig["rewriteTime"] = 0;
	}

	_switchOffOnBlack       = _devConfig[CONFIG_ON_OFF_BLACK].toBool(true);
	_blackLightsTimeout     = _devConfig[CONFIG_BLACK_LIGHTS_TIMEOUT].toInt(15000);
	_brightnessFactor       = _devConfig[CONFIG_BRIGHTNESSFACTOR].toDouble(1.0);
	_transitionTime         = _devConfig[CONFIG_TRANSITIONTIME].toInt(1);
	_isRestoreOrigState     = _devConfig[CONFIG_RESTORE_STATE].toBool(true);
	_groupId                = _devConfig[CONFIG_groupId].toString();
	_blackLevel             = _devConfig["blackLevel"].toDouble(0.0);
	_onBlackTimeToPowerOff  = _devConfig["onBlackTimeToPowerOff"].toInt(100);
	_onBlackTimeToPowerOn   = _devConfig["onBlackTimeToPowerOn"].toInt(100);
	_candyGamma             = _devConfig["candyGamma"].toBool(true);

	if (_blackLevel < 0.0) { _blackLevel = 0.0; }
	if (_blackLevel > 1.0) { _blackLevel = 1.0; }

	if (LedDevicePhilipsHueBridge::init(_devConfig))
	{
		log( "Off on Black", "%s", _switchOffOnBlack ? "Yes" : "No" );
		log( "Brightness Factor", "%f", _brightnessFactor );
		log( "Transition Time", "%d", _transitionTime );
		log( "Restore Original State", "%s", _isRestoreOrigState ? "Yes" : "No" );
		log( "Use Hue Entertainment API", "%s", _useEntertainmentAPI ? "Yes" : "No" );
		log("Brightness Threshold", "%f", _blackLevel);
		log("CandyGamma", "%s", _candyGamma ? "Yes" : "No" );
		log("Time powering off when black", "%s", _onBlackTimeToPowerOff ? "Yes" : "No" );
		log("Time powering on when signalled", "%s", _onBlackTimeToPowerOn ? "Yes" : "No" );

		if( _useEntertainmentAPI )
		{
			log( "Entertainment API Group-ID", "%s", QSTRING_CSTR(_groupId) );

			if( _groupId.isEmpty() )
			{
				Error(_log, "Disabling usage of Entertainment API - Group-ID is invalid [%s]", QSTRING_CSTR(_groupId) );
				_useEntertainmentAPI = false;
			}
		}

		// Everything is OK -> enable
		_isDeviceInitialised = true;
		isInitOK = true;
	}

	return isInitOK;
}

bool LedDevicePhilipsHue::setLights()
{
	bool isInitOK = true;

	_lightIds.clear();

	QStringList lights;

	if( _useEntertainmentAPI && !_groupId.isEmpty() )
	{
		lights = getGroupLights( _groupId );
	}

	if( lights.empty() )
	{
		if( _useEntertainmentAPI )
		{
			_useEntertainmentAPI = false;
			Error(_log, "Group-ID [%s] is not usable - Entertainment API usage was disabled!", QSTRING_CSTR(_groupId)  );
		}
		lights = _devConfig[ CONFIG_lightIdS ].toVariant().toStringList();
	}

	_lightIds = lights;
	int configuredLightsCount = _lightIds.size();

	if ( configuredLightsCount == 0 )
	{
		this->setInError( "No light-IDs configured" );
		isInitOK = false;
	}
	else
	{
		Debug(_log, "Lights configured: %d", configuredLightsCount );
		if (updateLights( getLightMap()))
		{
			if (_useApiV2)
			{
				_channelsCount = getGroupChannelsCount (_groupId);

				Debug(_log, "Channels configured: %d", _channelsCount );
				int ledsCount = getLedCount();
				if ( ledsCount == _channelsCount)
				{
					isInitOK = true;
				}
				else
				{
					QString errorText = QString("Number of hardware LEDs configured [%1] do not match the Entertainment lights' channel number [%2]."\
												" Please update your configuration.").arg(ledsCount).arg(_channelsCount );
					setInError(errorText, false);
					isInitOK = false;
				}
			}
		}
		else
		{
			isInitOK = false;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHue::initLeds()
{
	bool isInitOK = false;

	if ( !this->isInError() )
	{
		if( setLights() )
		{
			if( _useEntertainmentAPI )
			{
				_groupName = getGroupName( _groupId );
			}
			else
			{
				// adapt latchTime to count of user lightIds (bridge 10Hz max overall)
				setLatchTime( 100 * getLightsCount() );
				isInitOK = true;
			}
			_isInitLeds = true;
		}
		else
		{
			isInitOK = false;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHue::updateLights(const QMap<QString, QJsonObject> &map)
{
	bool isInitOK = true;

	// search user lightId inside map and create light if found
	_lights.clear();

	if(!_lightIds.empty())
	{
		_lights.reserve(static_cast<size_t>(_lightIds.size()));
		for(const auto &id : std::as_const(_lightIds))
		{
			if (map.contains(id))
			{
				_lights.emplace_back(_log, _useApiV2, id, map.value(id), _onBlackTimeToPowerOff, _onBlackTimeToPowerOn);
			}
			else
			{
				Warning(_log, "Configured light-ID %s is not available at this bridge", QSTRING_CSTR(id) );
			}
		}
	}

	int lightsCount = static_cast<int>(_lights.size());

	setLightsCount( lightsCount );

	if( lightsCount == 0 )
	{
		Error(_log, "No usable lights found!" );
		isInitOK = false;
	}
	else
	{
		//Populate additional light details
		int i {1};
		for (PhilipsHueLight& light : _lights)
		{
			light.setDeviceDetails(getDeviceDetails(light.getdeviceId()));
			light.setEntertainmentSrvDetails(getEntertainmentSrvDetails(light.getdeviceId()));

			Info(_log,"Light[%d]: \"%s\" [%s], Product: %s, Model: %s, Segments [%d]",
				 i,
				 QSTRING_CSTR(light.getName()),
				 QSTRING_CSTR(light.getId()),
				 QSTRING_CSTR(light.getProduct()),
				 QSTRING_CSTR(light.getModel()),
				 light.getMaxSegments()
				 );
			++i;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHue::openStream()
{
	bool isInitOK = false;
	bool streamState = getStreamGroupState();

	if ( !this->isInError() )
	{
		// stream is already active
		if( streamState )
		{
			// if same owner stop stream
			if(isStreamOwner(_streamOwner))
			{
				Debug(_log, "Group: \"%s\" [%s] is in use, try to stop stream", QSTRING_CSTR(_groupName), QSTRING_CSTR(_groupId) );

				if( stopStream() )
				{
					Debug(_log, "Stream successful stopped");
					//Restore Philips Hue devices state
					restoreState();
					isInitOK = startStream();
				}
				else
				{
					Error(_log, "Group: \"%s\" [%s] couldn't stop by user: \"%s\" - Entertainment API not usable", QSTRING_CSTR( _groupName ), QSTRING_CSTR(_groupId), QSTRING_CSTR( _streamOwner ) );
				}
			}
			else
			{
				Error(_log, "Group: \"%s\" [%s] is in use and owned by other user: \"%s\" - Entertainment API not usable", QSTRING_CSTR(_groupName), QSTRING_CSTR(_groupId), QSTRING_CSTR(_streamOwner));
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
			Info(_log, "Philips Hue Entertainment API successful connected! Start Streaming.");
		}
		else
		{
			Error(_log, "Philips Hue Entertainment API not connected!");
		}
	}
	else
	{
		Error(_log, "Philips Hue Entertainment API could not be initialized!");
	}
	return isInitOK;
}

bool LedDevicePhilipsHue::startStream()
{
	int retries {3};
	while (!setStreamGroupState(true) && --retries > 0)
	{
		Debug(_log, "Start Entertainment stream. Retrying...");
		QThread::msleep(500);
	}

	bool rc = (retries > 0);
	if (rc)
	{
		Debug(_log, "The Entertainment stream started successfully");
	}
	else
	{
		this->setInError("The Entertainment stream failed to start. Give up.");
	}
	return rc;
}

bool LedDevicePhilipsHue::stopStream()
{
	stopConnection();

	int retries = 3;
	while (!setStreamGroupState(false) && --retries > 0)
	{
		Debug(_log, "Stop Entertainment stream. Retrying...");
		QThread::msleep(500);
	}

	bool rc = (retries > 0);
	if (rc)
	{
		Debug(_log, "The Entertainment stream stopped successfully");
	}
	else
	{
		this->setInError("The Entertainment stream did NOT stop. Give up.");
	}
	return rc;
}

bool LedDevicePhilipsHue::getStreamGroupState()
{
	bool streamState {false};
	QJsonDocument doc = getGroupDetails( _groupId );
	DebugIf(verbose, _log, "GroupDetails: [%s]", QJsonDocument(doc).toJson(QJsonDocument::Compact).constData());

	if ( !this->isInError() )
	{
		if (_useApiV2)
		{
			QJsonArray groups = doc.array();
			if (groups.isEmpty())
			{
				this->setInError( "No Entertainment/Streaming details in Group found" );
			}
			else
			{
				QJsonObject group = groups[0].toObject();
				QString streamStaus = group.value(API_STREAM_STATUS).toString();
				if ( streamStaus == API_STREAM_ACTIVE)
				{
					streamState = true;
				}
				QJsonObject streamer = group.value(API_STREAM_ACTIVE_V2).toObject();
				_streamOwner = streamer[API_RID].toString();
			}
		}
		else
		{
			QJsonObject obj = doc.object()[ API_STREAM ].toObject();

			if( obj.isEmpty() )
			{
				this->setInError( "No Entertainment/Streaming details in Group found" );
			}
			else
			{
				streamState = obj.value( API_STREAM_ACTIVE ).toBool();
				_streamOwner = obj.value( API_OWNER ).toString();
			}
		}
	}
	return streamState;
}

bool LedDevicePhilipsHue::setStreamGroupState(bool state)
{
	QJsonDocument doc = setGroupState( _groupId, state );
	DebugIf(verbose, _log, "StreamGroupState: [%s]", QJsonDocument(doc).toJson(QJsonDocument::Compact).constData());

	if (_useApiV2)
	{
		if (doc.isEmpty())
		{
			_groupStreamState = false;
		}
		else
		{
			_groupStreamState = state;
		}
		return (_groupStreamState == state);
	}
	else
	{
		QJsonArray response = doc.array();
		if (!response.isEmpty())
		{
			QJsonObject msg = response.first().toObject();
			if ( !msg.contains( API_SUCCESS ) )
			{
				QString active = state ? API_STREAM_ACTIVE_VALUE_TRUE : API_STREAM_ACTIVE_VALUE_FALSE;
				Warning(_log, "%s", QSTRING_CSTR(QString("Set stream to %1: Neither error nor success contained in Bridge response...").arg(active)));
			}
			else
			{
				//Check original Hue response {"success":{"/groups/groupId/stream/active":activeYesNo}}
				QJsonObject success = msg.value(API_SUCCESS).toObject();
				QString valueName = QString( API_STREAM_RESPONSE_FORMAT ).arg( API_RESOURCE_GROUPS, _groupId, API_STREAM, API_STREAM_ACTIVE );
				QJsonValue result = success.value(valueName);
				if (result.isUndefined())
				{
					//Workaround
					//Check diyHue response   {"success":{"/groups/groupId/stream":{"active":activeYesNo}}}
					QString diyHueValueName = QString( "/%1/%2/%3" ).arg( API_RESOURCE_GROUPS, _groupId, API_STREAM);
					result = success.value(diyHueValueName).toObject().value(API_STREAM_ACTIVE);
				}

				_groupStreamState = result.toBool();
				return (_groupStreamState == state);
			}
		}
	}
	return false;
}

void LedDevicePhilipsHue::stop()
{
	LedDevicePhilipsHueBridge::stop();
}

int LedDevicePhilipsHue::open()
{
	int retval = -1;
	if ( LedDevicePhilipsHueBridge::open() == 0)
	{
		if (initLeds())
		{
			retval = 0;
		}
	}
	return retval;
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> & ledValues)
{
	// lights will be empty sometimes
	if( _lights.empty() )
	{
		return -1;
	}

	// more lights than LEDs, stop always
	if( static_cast<int>(ledValues.size()) < getLightsCount() )
	{
		Error(_log, "More light-IDs configured than LEDs, each light-ID requires one LED!" );
		return -1;
	}

	int rc {0};
	if (_isOn)
	{
		if (!_useApiV2)
		{
			rc = writeSingleLights( ledValues );
		}

		if (_useEntertainmentAPI && _isInitLeds)
		{
			rc= writeStreamData(ledValues);
		}
	}
	return rc;
}

int LedDevicePhilipsHue::writeSingleLights(const std::vector<ColorRgb>& ledValues)
{
	// Iterate through lights and set colors.
	unsigned int idx = 0;
	for ( PhilipsHueLight& light : _lights )
	{
		// Get color.
		ColorRgb color = ledValues.at(idx);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		CiColor xy = CiColor::rgbToCiColor(color.red / 255.0, color.green / 255.0, color.blue / 255.0, light.getColorSpace(), _candyGamma);

		if (_switchOffOnBlack && xy.bri <= _blackLevel && light.isBlack(true))
		{
			xy.bri = 0;
			xy.x = 0;
			xy.y = 0;

			if( _useEntertainmentAPI )
			{
				if (light.getOnOffState())
				{
					this->setColor(light, xy);
					this->setOnOffState(light, false);
				}
			}
			else
			{
				if (light.getOnOffState())
				{
					setState(light, false, xy);
				}
			}
		}
		else
		{
			bool currentstate = light.getOnOffState();

			if (_switchOffOnBlack && xy.bri > _blackLevel && light.isWhite(true))
			{
				if (!currentstate)
				{
					xy.bri = xy.bri / 2;
				}

				if (_useEntertainmentAPI)
				{
					this->setOnOffState(light, true);
					this->setColor(light, xy);
				}
				else
				{
					this->setState(light, true, xy);
				}
			}
			else if (!_switchOffOnBlack)
			{
				// Write color if color has been changed.
				if (_useEntertainmentAPI)
				{
					this->setOnOffState(light, true);
					this->setColor(light, xy);
				}
				else
				{
					this->setState( light, true, xy );
				}
			}
		}
		if (xy.bri > _blackLevel)
		{
			light.isBlack(false);
		}
		else if (xy.bri <= _blackLevel)
		{
			light.isWhite(false);
		}

		++idx;
	}
	return 0;
}

int LedDevicePhilipsHue::writeStreamData(const std::vector<ColorRgb>& ledValues, bool flush)
{
	QByteArray msg;

	if (_useApiV2)
	{
		int ledsCount = static_cast<int>(ledValues.size());
		if ( ledsCount != _channelsCount )
		{
			QString errorText = QString("Number of LEDs configured via the layout [%1] do not match the Entertainment lights' channel number [%2]."\
										" Please update your configuration.").arg(ledsCount).arg(_channelsCount);
			this->setInError(errorText, false);
			return -1;
		}

		//		"HueStream", //protocol
		//		0x02, 0x00, //version 2.0
		//		0x07, //sequence number 7
		//		0x00, 0x00, //reserved
		//		0x00, //color mode RGB
		//		0x00, //reserved
		//		"1a8d99cc-967b-44f2-9202-43f976c0fa6b", //entertainment configuration id
		//		0x00, //channel id 0
		//		0xff, 0xff, 0x00, 0x00, 0x00, 0x00, //red
		//		0x01, //channel id 1
		//		0x00, 0x00, 0xff, 0xff, 0x00, 0x00 //green
		//		0x02, //channel id 2
		//		0x00, 0x00, 0x00, 0x00, 0xff, 0xff //blue
		//		0x03, //channel id 3
		//		0xff, 0xff, 0xff, 0xff, 0xff, 0xff //white
		//		//etc for channel ids 4-7

		msg.reserve(static_cast<int>(sizeof(HEADER_V2) + sizeof(ENTERTAINMENT_ID) + sizeof(PAYLOAD_PER_CHANNEL_V2) * _lights.size()));
		msg.append(reinterpret_cast<const char*>(HEADER_V2), sizeof(HEADER_V2));
		msg.append(_groupId.toLocal8Bit());

		uint8_t maxChannels = static_cast<uint8_t>(ledValues.size());

		ColorRgb color;

		for (uint8_t channel = 0; channel < maxChannels; ++channel)
		{
			if (channel < 20) // v2 max 20 channels
			{
				color = static_cast<ColorRgb>(ledValues.at(channel));

				quint16 R = static_cast<quint16>(color.red << 8);
				quint16 G = static_cast<quint16>(color.green << 8);
				quint16 B = static_cast<quint16>(color.blue<< 8);

				msg.append(static_cast<char>(channel));
				const uint16_t payload[] = { qToBigEndian<quint16>(R), qToBigEndian<quint16>(G), qToBigEndian<quint16>(B) };
				msg.append(reinterpret_cast<const char *>(payload), sizeof(payload));
			}
		}
	}
	else
	{
		//		"HueStream", //protocol
		//		0x01, 0x00, //version 1.0
		//		0x07, //sequence number 7
		//		0x00, 0x00, //reserved
		//		0x00, //color mode RGB
		//		0x00, //reserved
		//		0x00, 0x00, 0x01, //light ID 1
		//		0xff, 0xff, 0x00, 0x00, 0x00, 0x00, //red
		//		0x00, 0x00, 0x04, //light ID 4
		//		0x00, 0x00, 0x00, 0x00, 0xff, 0xff //blue

		msg.reserve(static_cast<int>(sizeof(HEADER) + sizeof(PAYLOAD_PER_LIGHT) * _lights.size()));
		msg.append(reinterpret_cast<const char*>(HEADER), sizeof(HEADER));

		ColorRgb color;

		uint8_t i = 0;
		for (const PhilipsHueLight& light : _lights)
		{
			if (i < 10) // max 10 lights
			{
				uint8_t id = static_cast<uint8_t>(light.getId().toInt());

				color = static_cast<ColorRgb>(ledValues.at(i));
				quint16 R = static_cast<quint16>(color.red << 8);
				quint16 G = static_cast<quint16>(color.green << 8);
				quint16 B = static_cast<quint16>(color.blue<< 8);

				msg.append(2, 0x00);
				msg.append(static_cast<char>(id));
				const uint16_t payload[] = {
					qToBigEndian<quint16>(R), qToBigEndian<quint16>(G),	qToBigEndian<quint16>(B)
				};
				msg.append(reinterpret_cast<const char *>(payload), sizeof(payload));
			}
			++i;
		}
	}

	if (verbose3) {
		qDebug() << "Msg Hex:" << msg.toHex(':');
	}

	writeBytes(msg, flush);
	return 0;
}

void LedDevicePhilipsHue::setOnOffState(PhilipsHueLight& light, bool on, bool force)
{
	if (light.getOnOffState() != on || force)
	{
		Debug(_log,"id: %s, on: %d", QSTRING_CSTR(light.getId()), on);
		QStringList resourcePath;
		QJsonObject cmd;

		if (_useApiV2)
		{
			resourcePath << API_RESOURCE_LIGHT << light.getId();
			cmd.insert(API_STATE_ON, QJsonObject {{API_STATE_ON, on }});
		}
		else
		{
			resourcePath << API_RESOURCE_LIGHTS << light.getId() << API_STATE;
			cmd.insert(API_STATE_ON, on);
		}
		put(resourcePath, cmd);

		if (!isInError())
		{
			light.setOnOffState( on );
		}
	}
}

void LedDevicePhilipsHue::setTransitionTime(PhilipsHueLight& light)
{
	if (light.getTransitionTime() != _transitionTime)
	{
		QStringList resourcePath;
		QJsonObject cmd;

		if (_useApiV2)
		{
			resourcePath << API_RESOURCE_LIGHT << light.getId();
			cmd.insert(API_DYNAMICS, QJsonObject {{API_DURATION, _transitionTime }});
		}
		else
		{
			resourcePath << API_RESOURCE_LIGHTS << light.getId() << API_STATE;
			cmd.insert(API_TRANSITIONTIME, _transitionTime);
		}
		put(resourcePath, cmd);

		if (!isInError())
		{
			light.setTransitionTime( _transitionTime );
		}
	}
}

void LedDevicePhilipsHue::setColor(PhilipsHueLight& light, CiColor& color)
{
	if (!light.hasColor() || light.getColor() != color)
	{
		if( !_useEntertainmentAPI )
		{
			QStringList resourcePath;
			QJsonObject cmd;

			if (!light.hasColor() || light.getColor() != color)
			{
				if (_useApiV2)
				{
					resourcePath << API_RESOURCE_LIGHT << light.getId();

					// Brightness is 0-100 %, Brightness percentage. value cannot be 0, writing 0 changes it to lowest possible brightness
					const double bri = qMin(_brightnessFactor *  color.bri * 100, 100.0);

					QJsonObject colorXY;
					colorXY[API_X_COORDINATE] = color.x;
					colorXY[API_Y_COORDINATE] = color.y;
					cmd.insert(API_COLOR, QJsonObject {{API_DURATION, colorXY }});
					cmd.insert(API_DIMMING, QJsonObject {{API_BRIGHTNESS, bri }});
				}
				else
				{
					resourcePath << API_RESOURCE_LIGHTS << light.getId() << API_STATE;

					const int bri = qRound(qMin(254.0, _brightnessFactor * qMax(1.0, color.bri * 254.0)));
					QJsonArray colorXY;
					colorXY.append(color.x);
					colorXY.append(color.y);
					cmd.insert(API_XY_COORDINATES, colorXY);
					cmd.insert(API_BRIGHTNESS, bri);
				}
			}
			put(resourcePath, cmd);
		}
		else
		{
			color.bri = (qMin(1.0, _brightnessFactor * qMax(0.0, color.bri)));
		}

		if (!isInError())
		{
			light.setColor( color );
		}
	}
}

void LedDevicePhilipsHue::setState(PhilipsHueLight& light, bool on, const CiColor& color)
{
	QStringList resourcePath;
	QJsonObject cmd;
	bool forceCmd {false};

	if (light.getOnOffState() != on)
	{
		forceCmd = true;
		if (_useApiV2)
		{
			cmd.insert(API_STATE_ON, QJsonObject {{API_STATE_ON, on }});
		}
		else
		{
			cmd.insert(API_STATE_ON, on);
		}
	}

	if (!_useEntertainmentAPI && light.getOnOffState())
	{
		if (light.getTransitionTime() != _transitionTime)
		{
			if (_useApiV2)
			{
				cmd.insert(API_DYNAMICS, QJsonObject {{API_DURATION, _transitionTime }});
			}
			else
			{
				cmd.insert(API_TRANSITIONTIME, _transitionTime);
			}
		}

		if (!light.hasColor() || light.getColor() != color)
		{
			if (!light.isBusy() || forceCmd)
			{
				if (_useApiV2)
				{
					// Brightness is 0-100 %, Brightness percentage. value cannot be 0, writing 0 changes it to lowest possible brightness
					const double bri = qMin(_brightnessFactor *  color.bri * 100, 100.0);

					QJsonObject colorXY;
					colorXY[API_X_COORDINATE] = color.x;
					colorXY[API_Y_COORDINATE] = color.y;
					cmd.insert(API_COLOR, QJsonObject {{API_DURATION, colorXY }});
					cmd.insert(API_DIMMING, QJsonObject {{API_BRIGHTNESS, bri }});
				}
				else
				{
					const int bri = qRound(qMin(254.0, _brightnessFactor * qMax(1.0, color.bri * 254.0)));
					QJsonArray colorXY;
					colorXY.append(color.x);
					colorXY.append(color.y);
					cmd.insert(API_XY_COORDINATES, colorXY);
					cmd.insert(API_BRIGHTNESS, bri);
				}
			}
		}
	}

	if (!cmd.isEmpty())
	{
		if (_useApiV2)
		{
			resourcePath << API_RESOURCE_LIGHT << light.getId();
		}
		else
		{
			resourcePath << API_RESOURCE_LIGHTS << light.getId() << API_STATE;
		}
		put(resourcePath, cmd);

		if (!isInError())
		{
			light.setTransitionTime( _transitionTime );
			light.setColor( color );
			light.setOnOffState( on );
		}
	}
}

void LedDevicePhilipsHue::setLightsCount(int lightsCount)
{
	_lightsCount = lightsCount;
}


bool LedDevicePhilipsHue::switchOn()
{
	bool rc {false};

	if ( _isOn )
	{
		Debug(_log, "Device %s is already on. Skipping.", QSTRING_CSTR(_activeDeviceType));
		rc = true;
	}
	else
	{
		if ( _isDeviceReady )
		{
			Info(_log, "Switching device %s ON", QSTRING_CSTR(_activeDeviceType));
			if ( storeState() )
			{
				if (_useEntertainmentAPI)
				{
					if (openStream())
					{
						if (startConnection())
						{
							if ( (!_useApiV2 || _isDiyHue) ) //DiyHue does not auto switch on, if stream starts
							{
								powerOn();
							}

							_isOn = true;
							setRewriteTime(STREAM_REWRITE_TIME.count());
						}
					}
					else
					{
						// TODO: Failed to OpenStream - should retry
					}
				}
				else
				{
					if ( powerOn() )
					{
						_isOn = true;
					}
				}
			}

			if (_isOn)
			{
				Info(_log, "Device %s is ON", QSTRING_CSTR(_activeDeviceType));
				emit enableStateChanged(_isEnabled);
				rc =true;
			}
			else
			{
				Warning(_log, "Failed switching device %s ON", QSTRING_CSTR(_activeDeviceType));
			}
		}
	}
	return rc;
}

bool LedDevicePhilipsHue::switchOff()
{
	bool rc {false};

	if ( !_isOn )
	{
		rc = true;
	}
	else
	{
		if ( _isDeviceInitialised )
		{
			if ( _isDeviceReady )
			{
				if (_useEntertainmentAPI && _groupStreamState)
				{
					Info(_log, "Switching device %s OFF", QSTRING_CSTR(_activeDeviceType));

					setRewriteTime(0);

					if ( _isRestoreOrigState )
					{
						_isOn = false;
						stopStream();
						rc = restoreState();
					}
					else
					{
						_isOn = false;
						rc = stopStream();

						if ( (!_useApiV2 || _isDiyHue) ) //DiyHue does not auto switch off, if stream stopps
						{
							rc = powerOff();
						}
					}

					if (rc)
					{
						Info(_log, "Device %s is OFF", QSTRING_CSTR(_activeDeviceType));
						rc = true;
					}
					else
					{
						Warning(_log, "Failed switching device %s OFF", QSTRING_CSTR(_activeDeviceType));
					}

				}
				else
				{
					Debug(_log,"LedDevicePhilipsHueBridge::switchOff()");
					rc = LedDevicePhilipsHueBridge::switchOff();
				}
			}
			else
			{
				rc = true;
			}
		}
	}
	return rc;
}

bool LedDevicePhilipsHue::powerOn()
{
	bool rc {true};
	if (_isDeviceReady)
	{
		for ( PhilipsHueLight& light : _lights )
		{
			setOnOffState(light, true, true);
		}
	}
	return rc;
}

bool LedDevicePhilipsHue::powerOff()
{
	bool rc {true};
	if (_isDeviceReady)
	{
		for ( PhilipsHueLight& light : _lights )
		{
			setOnOffState(light, false, true);
		}
	}
	return rc;
}

bool LedDevicePhilipsHue::storeState()
{
	bool rc {true};
	if ( _isRestoreOrigState )
	{
		if( !_lightIds.empty() )
		{
			for ( PhilipsHueLight& light : _lights )
			{
				QJsonObject values = getLightDetails(light.getId());
				light.saveOriginalState(values);
			}
		}
	}
	return rc;
}

bool LedDevicePhilipsHue::restoreState()
{
	bool rc {true};
	if ( _isRestoreOrigState )
	{
		// Restore device's original state
		if( !_lightIds.empty() )
		{
			for ( PhilipsHueLight& light : _lights )
			{
				setLightState( light.getId(),light.getOriginalState() );
			}
		}
	}
	return rc;
}

void LedDevicePhilipsHue::identify(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QJsonDocument(params).toJson(QJsonDocument::Compact).constData());
	QJsonObject properties;

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort =  params[CONFIG_PORT].toInt();
	_authToken = params[CONFIG_USERNAME].toString("");

	QString lighName = params["lightName"].toString();

	Info(_log, "Identify %s, Light: \"%s\" @hostname (%s)", QSTRING_CSTR(_activeDeviceType),  QSTRING_CSTR(lighName), QSTRING_CSTR(_hostName) );

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address))
	{
		QJsonDocument bridgeDetails = retrieveBridgeDetails();
		if ( !bridgeDetails.isEmpty() )
		{
			setBridgeDetails(bridgeDetails);
			if ( openRestAPI() )
			{
				_useApiV2 = _isAPIv2Ready;

				// DIYHue does not provide v2 Breathe effects, yet -> fall back to v1
				if (_isDiyHue)
				{
					_useApiV2 = false;
				}

				if (_useApiV2)
				{
					configureSsl();
				}

				if (!isInError() )
				{
					setBaseApiEnvironment(_useApiV2);

					QStringList resourcepath;
					QJsonObject cmd;
					if (_useApiV2)
					{
						QString lightId = params[API_LIGTH_ID].toString();
						resourcepath << API_RESOURCE_LIGHT << lightId;
						cmd.insert(API_ALERT, QJsonObject {{API_ACTION, API_ACTION_BREATHE}});
					}
					else
					{
						bool on {true};
						QString lightId = params[API_LIGTH_ID_v1].toString();
						resourcepath << lightId << API_STATE;
						cmd.insert(API_STATE_ON, on);
						cmd.insert(API_ALERT, API_SELECT);
					}
					_restApi->setPath(resourcepath);

					// Perform request
					httpResponse response = _restApi->put(cmd);
					if (response.error())
					{
						Warning(_log, "%s identification failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
					}
				}
			}
		}
	}
}
