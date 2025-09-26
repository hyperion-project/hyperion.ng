// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

#include <chrono>
#include <QStringLiteral>
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
namespace
{

	bool verbose = false;
	const bool verbose3 = false;

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
	const char CONFIG_LIGHTIDS[] = "lightIds";
	const char CONFIG_USE_HUE_API_V2[] = "useAPIv2";
	const char CONFIG_USE_HUE_ENTERTAINMENT_API[] = "useEntertainmentAPI";
	const char CONFIG_groupId[] = "groupId";

	const char CONFIG_VERBOSE[] = "verbose";

	// Philips Hue OpenAPI URLs
	const int API_DEFAULT_PORT = -1; // Use default port per communication scheme
	const char API_ROOT[] = "/";
	const char API_BASE_PATH_V1[] = "api";
	const char API_BASE_PATH_V2[] = "/clip/v2/resource";
	const char API_AUTH_PATH_V1[] = "auth/v1";
	const char API_RESOURCE_CONFIG[] = "config";
	const char API_RESOURCE_LIGHTS[] = "lights";
	const char API_RESOURCE_GROUPS[] = "groups";
	// V2
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

	// MAC prefixes for Philips Bridge V1, Bridge V2, Bridge Pro
	const QStringList DEV_DATA_MAC_PREFIXES = {"001788", "ECB5FA", "C42996"};

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
	const int SSL_CIPHERSUITES[2] = {MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256, 0};

	const int DEV_FIRMWAREVERSION_APIV2 = 1948086000;

	// Enable rewrites that Hue-Bridge does not close the connection ("After 10 seconds of no activity the connection is closed automatically, and status is set back to inactive.")
	constexpr std::chrono::milliseconds STREAM_REWRITE_TIME{5000};

	// Streaming message header and payload definition
	const uint8_t HEADER[] =
		{
			'H', 'u', 'e', 'S', 't', 'r', 'e', 'a', 'm', // protocol
			0x01, 0x00,									 // version 1.0
			0x01,										 // sequence number 1
			0x00, 0x00,									 // Reserved write 0’s
			0x00,										 // 0x00 = RGB; 0x01 = XY Brightness
			0x00,										 // Reserved, write 0’s
	};

	const uint8_t PAYLOAD_PER_LIGHT[] =
		{
			0x01, 0x00, 0x06, // light ID
			// color: 16 bpc
			0xff, 0xff, // Red
			0xff, 0xff, // Green
			0xff, 0xff, // Blue
	};

	// API v2 - Streaming message header and payload definition
	const uint8_t HEADER_V2[] =
		{
			'H', 'u', 'e', 'S', 't', 'r', 'e', 'a', 'm', // protocol
			0x02, 0x00,									 // version 2.0
			0x01,										 // sequence number 1
			0x00, 0x00,									 // Reserved write 0’s
			0x00,										 // 0x00 = RGB; 0x01 = XY Brightness
			0x00,										 // Reserved
	};

	const char *ENTERTAINMENT_ID[36];
	const uint8_t PAYLOAD_PER_CHANNEL_V2[] =
		{
			0xff,							   // channel id
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff // color
	};

} // End of constants

bool operator==(const CiColor &p1, const CiColor &p2)
{
	return ((p1.x == p2.x) && (p1.y == p2.y) && (p1.bri == p2.bri));
}

bool operator!=(const CiColor &p1, const CiColor &p2)
{
	return !(p1 == p2);
}

CiColor CiColor::rgbToCiColor(double red, double green, double blue, const CiColorTriangle &colorSpace, bool candyGamma)
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

	CiColor xy = {cx, cy, bri};

	if ((red + green + blue) > 0)
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
	XYColor v1 = {colorSpace.green.x - colorSpace.red.x, colorSpace.green.y - colorSpace.red.y};
	XYColor v2 = {colorSpace.blue.x - colorSpace.red.x, colorSpace.blue.y - colorSpace.red.y};
	XYColor q = {p.x - colorSpace.red.x, p.y - colorSpace.red.y};
	double s = crossProduct(q, v2) / crossProduct(v1, v2);
	double t = crossProduct(v1, q) / crossProduct(v1, v2);
	if ((s >= 0.0) && (t >= 0.0) && (s + t <= 1.0))
	{
		rc = true;
	}
	return rc;
}

XYColor CiColor::getClosestPointToPoint(XYColor a, XYColor b, CiColor p)
{
	XYColor AP = {p.x - a.x, p.y - a.y};
	XYColor AB = {b.x - a.x, b.y - a.y};
	double ab2 = AB.x * AB.x + AB.y * AB.y;
	double ap_ab = AP.x * AB.x + AP.y * AB.y;
	double t = ap_ab / ab2;
	if (t < 0.0)
	{
		t = 0.0;
	}
	else if (t > 1.0)
	{
		t = 1.0;
	}
	return {a.x + AB.x * t, a.y + AB.y * t};
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
	: ProviderUdpSSL(deviceConfig), _restApi(nullptr), _apiPort(API_DEFAULT_PORT), _api_major(0), _api_minor(0), _api_patch(0), _isPhilipsHueBridge(false), _isDiyHue(false), _isHueEntertainmentReady(false), _isAPIv2Ready(false), _useEntertainmentAPI(false), _useApiV2(true)
{
#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(MdnsBrowser::getInstance().data(), "browseForServiceType",
							  Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType(_activeDeviceType)));
#endif
}

LedDevicePhilipsHueBridge::~LedDevicePhilipsHueBridge()
{
	qDebug() << "LedDevicePhilipsHueBridge::~LedDevicePhilipsHueBridge()";
}

bool LedDevicePhilipsHueBridge::init(const QJsonObject &deviceConfig)
{
	DebugIf(verbose, _log, "deviceConfig: [%s]", QJsonDocument(_devConfig).toJson(QJsonDocument::Compact).constData());

	bool isInitOK = false;

	// Set hostname as per configuration and default port
	_hostName = deviceConfig[CONFIG_HOST].toString();
	_apiPort = deviceConfig[CONFIG_PORT].toInt();
	setBridgeId(deviceConfig[DEV_DATA_BRIDGEID].toString());
	_authToken = deviceConfig[CONFIG_USERNAME].toString();

	Debug(_log, "Hostname/IP: %s", QSTRING_CSTR(_hostName));
	Debug(_log, "Bridge-ID: %s", QSTRING_CSTR(getBridgeId()));

	_useApiV2 = deviceConfig[CONFIG_USE_HUE_API_V2].toBool(true);
	Debug(_log, "Use Hue API v2: %s", _useApiV2 ? "Yes" : "No");

	if (_useEntertainmentAPI)
	{
		setLatchTime(0);
		_devConfig["sslport"] = API_SSL_SERVER_PORT;
		_devConfig["servername"] = API_SSL_SERVER_NAME;
		_devConfig["psk"] = _devConfig[CONFIG_CLIENTKEY].toString();
		if (_useApiV2)
		{
			// psk_identity is to be set later when application-id was resolved
			_devConfig["psk_identity"] = "";
		}
		else
		{
			_devConfig["psk_identity"] = _authToken;
		}
		_devConfig["seed_custom"] = API_SSL_SEED_CUSTOM;
		_devConfig["retry_left"] = STREAM_CONNECTION_RETRYS;
		_devConfig["hs_attempts"] = STREAM_SSL_HANDSHAKE_ATTEMPTS;
		_devConfig["hs_timeout_min"] = 600;
		_devConfig["hs_timeout_max"] = 1000;

		_port = API_SSL_SERVER_PORT;

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
	bool isInitOK{true};

	if (_hostName.isNull())
	{
		Error(_log, "Empty hostname or IP address. REST API cannot be initiatised.");
		return false;
	}

	if (_restApi.isNull())
	{
		_restApi.reset(new ProviderRestApi(_hostName, _apiPort));
		_restApi->setLogger(_log);
	}
	else
	{
		_restApi->setHost(_hostName);
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

bool LedDevicePhilipsHueBridge::handleV2ApiError(const QJsonObject &obj, QString &errorReason) const
{
	if (!obj.contains(API_ERRORS))
		return false;

	const QJsonArray errorList = obj.value(API_ERRORS).toArray();
	if (errorList.isEmpty())
		return false;

	QStringList errors;
	for (const QJsonValue &error : errorList)
	{
		QString errorString = error.toObject()[API_ERROR_DESCRIPTION].toString();
		if (!errorString.contains("may not have effect"))
		{
			errors << errorString;
		}
	}

	if (errors.isEmpty())
		return false;

	errorReason = errors.join(",");
	return true;
}

bool LedDevicePhilipsHueBridge::handleV1ApiError(const QJsonArray &responseList, QString &errorReason) const
{
	if (responseList.isEmpty())
		return false;

	QJsonObject response = responseList.first().toObject();
	if (!response.contains(API_ERROR))
		return false;

	QJsonObject error = response.value(API_ERROR).toObject();
	int errorType = error.value(API_ERROR_TYPE).toInt();

	if (errorType == 901) // Internal error, 901, Bridge likely rebooting
		return false;

	QString errorDesc = error.value(API_ERROR_DESCRIPTION).toString();
	QString errorAddress = error.value(API_ERROR_ADDRESS).toString();
	errorReason = QString("(%1) %2, Resource:%3").arg(errorType).arg(errorDesc, errorAddress);
	return true;
}

bool LedDevicePhilipsHueBridge::checkApiError(const QJsonDocument &response, bool supressError)
{
	bool apiError = false;
	QString errorReason;

	DebugIf(verbose, _log, "Reply: [%s]", response.toJson(QJsonDocument::Compact).constData());

	if (_useApiV2)
	{
		apiError = handleV2ApiError(response.object(), errorReason);
	}
	else
	{
		apiError = handleV1ApiError(response.array(), errorReason);
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
	_isDeviceReady = false;
	this->setIsRecoverable(true);

	NetUtils::resolveMdnsHost(_log, _hostName, _apiPort);

	if (!openRestAPI())
	{
		return -1;
	}

	QJsonDocument bridgeDetails = retrieveBridgeDetails();
	if (bridgeDetails.isEmpty())
	{
		Error(_log, "%s failed to get properties for bridge-id: [%s]. Unable to retrieve required bridge details.", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(getBridgeId()));
		return -1;
	}

	setBridgeDetails(bridgeDetails, true);

	if (_useApiV2)
	{
		if (configureSsl() && retrieveApplicationId())
		{
			setPSKidentity(_applicationID);
		}
	}
	else if (_isAPIv2Ready)
	{
		Warning(_log, "Your Hue Bridge supports a newer API. Reconfigure your device in Hyperion to benefit from new features.");
	}

	if (isInError())
	{
		return -1;
	}

	setBaseApiEnvironment(_useApiV2);
	if (!initLightsMap() || !initDevicesMap() || !initEntertainmentSrvsMap())
	{
		return -1;
	}

	if (_useEntertainmentAPI)
	{
		if (initGroupsMap() && ProviderUdpSSL::open() == 0)
		{
			_isDeviceReady = true;
			return 0;
		}
	}
	else
	{
		_isDeviceReady = true;
		return 0;
	}

	return -1;
}

int LedDevicePhilipsHueBridge::close()
{
	_isDeviceReady = false;
	int retval = 0;

	if (_useEntertainmentAPI)
	{
		retval = ProviderUdpSSL::close();
	}

	return retval;
}

bool LedDevicePhilipsHueBridge::configureSsl()
{
	if (_isPhilipsHueBridge)
	{
		if (_deviceBridgeId.isEmpty())
		{
			this->setInError("Failed to configure Hue Bridge for SSL, Bridge-ID is empty", false);
			return false;
		}

		// Do not allow self-signed certificates for official Hue Bridges
		// see https://developers.meethue.com/develop/application-design-guidance/using-https/
		_restApi->acceptSelfSignedCertificates(false);
	}
	else
	{
		_restApi->acceptSelfSignedCertificates(true);
	}

	_restApi->setAlternateServerIdentity(getBridgeId());

	bool success = _restApi->setCaCertificate(API_SSL_CA_CERTIFICATE_RESSOURCE);
	if (!success)
	{
		this->setInError("Failed to configure Hue Bridge for SSL", false);
	}

	return success;
}

const int *LedDevicePhilipsHueBridge::getCiphersuites() const
{
	return SSL_CIPHERSUITES;
}

void LedDevicePhilipsHueBridge::log(const QString& msg, const QVariant& value) const
{
	Debug(_log, "%s: %s", QSTRING_CSTR(msg.leftJustified(30, ' ')), QSTRING_CSTR(value.toString()));
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveBridgeDetails()
{
	QJsonDocument bridgeDetails;

	if (openRestAPI())
	{
		// Allow http fall-back only for DiyHue or other 3rd party bridges
		// Official Philips Hue Bridges should support API v2 and https
		if (_deviceBridgeId.isEmpty() && !_isPhilipsHueBridge)
		{
			Debug(_log, "Bridge-ID not available. Get bridge details via http call.");
			setBaseApiEnvironment(false, API_BASE_PATH_V1);
		}
		else
		{
			if (_isPhilipsHueBridge)
			{
				useApiV2(true);
			}

			if (!configureSsl())
			{
				return {};
			}
			setBaseApiEnvironment(_useApiV2, API_BASE_PATH_V1);
		}
		bridgeDetails = get(API_RESOURCE_CONFIG);
	}

	return bridgeDetails;
}

bool LedDevicePhilipsHueBridge::retrieveApplicationId()
{
	bool rc{false};

	setBaseApiEnvironment(true, API_ROOT);
	_restApi->setPath(API_AUTH_PATH_V1);

	httpResponse response = _restApi->get();

	if (!response.error())
	{
		_applicationID = response.getHeader("hue-application-id");
		rc = true;
	}
	else
	{
		QString errorReason = QString("Failed to get application-id from Hue Bridge, error: '%1'").arg(response.getErrorReason());
		this->setInError(errorReason, false);
	}
	return rc;
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveDeviceDetails(const QString &deviceId)
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
	return get(resourcePath);
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveLightDetails(const QString &lightId)
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
	return get(resourcePath);
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveGroupDetails(const QString &groupId)
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
	return get(resourcePath);
}

QJsonDocument LedDevicePhilipsHueBridge::retrieveEntertainmentSrvDetails(const QString &entertainmentID)
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
	return get(resourcePath);
}

bool LedDevicePhilipsHueBridge::isPhilipsHueBridge() const { return _isPhilipsHueBridge; }
bool LedDevicePhilipsHueBridge::isDiyHue() const { return _isDiyHue; }

bool LedDevicePhilipsHueBridge::isApiEntertainmentReady(const QString &apiVersion)
{
	bool ready{false};

	QStringList apiVersionParts = QStringUtils::split(apiVersion, ".", QStringUtils::SplitBehavior::SkipEmptyParts);
	if (!apiVersionParts.isEmpty())
	{
		_api_major = apiVersionParts[0].toUInt();
		_api_minor = apiVersionParts[1].toUInt();
		_api_patch = apiVersionParts[2].toUInt();

		if (_api_major > 1 || (_api_major == 1 && _api_minor >= 22))
		{
			ready = true;
		}
	}
	Debug(_log, "API version [%s] %s Entertainment API ready", QSTRING_CSTR(apiVersion), ready ? "is" : "is not");
	return ready;
}

bool LedDevicePhilipsHueBridge::isAPIv2Ready() const
{
	return _isAPIv2Ready;
}

bool LedDevicePhilipsHueBridge::isAPIv2Ready(int swVersion) const
{
	bool ready{true};
	if (swVersion < DEV_FIRMWAREVERSION_APIV2)
	{
		ready = false;
	}
	Debug(_log, "Firmware version [%d] %s API v2 ready", swVersion, ready ? "is" : "is not");
	return ready;
}

int LedDevicePhilipsHueBridge::getFirmwareVersion() const
{
	return _deviceFirmwareVersion;
}

void LedDevicePhilipsHueBridge::useEntertainmentAPI(bool useEntertainmentAPI)
{
	_useEntertainmentAPI = useEntertainmentAPI;
}

bool LedDevicePhilipsHueBridge::isUsingEntertainmentApi() const
{
	return _useEntertainmentAPI;
}

void LedDevicePhilipsHueBridge::useApiV2(bool useApiV2)
{
	_useApiV2 = useApiV2;
}

bool LedDevicePhilipsHueBridge::isUsingApiV2() const
{
	return _useApiV2;
}

void LedDevicePhilipsHueBridge::setBaseApiEnvironment(bool apiV2, const QString &path)
{
	if (!_restApi.isNull())
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
				// Base-path is api-path + authentication token (here username)
				basePath << API_BASE_PATH_V1 << _authToken;
			}
		}
		_restApi->setBasePath(basePath);

		DebugIf(verbose, _log, "New BasePath: %s", QSTRING_CSTR(_restApi->getBasePath()));
	}
}

bool LedDevicePhilipsHueBridge::initDevicesMap()
{
	bool isInitOK = false;

	if (!this->isInError())
	{
		QJsonDocument deviceDetails = retrieveDeviceDetails();
		if (!deviceDetails.isEmpty())
		{
			setDevicesMap(deviceDetails);
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::initLightsMap()
{
	bool isInitOK = false;

	if (!this->isInError())
	{
		QJsonDocument lightDetails = retrieveLightDetails();
		if (!lightDetails.isEmpty())
		{
			setLightsMap(lightDetails);
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::initGroupsMap()
{
	bool isInitOK = false;

	if (!this->isInError())
	{
		QJsonDocument groupDetails = retrieveGroupDetails();
		if (!groupDetails.isEmpty())
		{
			setGroupMap(groupDetails);
			isInitOK = true;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::initEntertainmentSrvsMap()
{
	bool isInitOK = false;

	if (!this->isInError())
	{
		QJsonDocument entertainmentSrvDetails = retrieveEntertainmentSrvDetails();
		if (!entertainmentSrvDetails.isEmpty())
		{
			setEntertainmentSrvMap(entertainmentSrvDetails);
			isInitOK = true;
		}
	}
	return isInitOK;
}

void LedDevicePhilipsHueBridge::setBridgeDetails(const QJsonDocument &doc, bool isLogging)
{
	QJsonObject jsonConfigInfo = doc.object();
	if (verbose)
	{
		std::cout << "jsonConfigInfo: [" << QJsonDocument(jsonConfigInfo).toJson(QJsonDocument::Compact).constData() << "]" << std::endl;
	}

	_deviceName = jsonConfigInfo[DEV_DATA_NAME].toString();
	_deviceModel = jsonConfigInfo[DEV_DATA_MODEL_V1].toString();
	setBridgeId(jsonConfigInfo[DEV_DATA_BRIDGEID].toString());
	_deviceFirmwareVersion = jsonConfigInfo[DEV_DATA_SOFTWAREVERSION].toString().toInt();
	_deviceAPIVersion = jsonConfigInfo[DEV_DATA_APIVERSION].toString();

	// Check if bridge-id MAC prefix is known from Philips Hue devices
	// If not, we assume it is a DiyHue or other 3rd party bridge
	if (!DEV_DATA_MAC_PREFIXES.contains(getBridgeId().left(6)))
	{
		_isPhilipsHueBridge = false;
	}

	// Check if bridge is DIYHue to apply workarounds
	if (_deviceName.startsWith("DiyHue", Qt::CaseInsensitive))
	{
		_isDiyHue = true;
	}

	_isHueEntertainmentReady = isApiEntertainmentReady(_deviceAPIVersion);
	_isAPIv2Ready = isAPIv2Ready(_deviceFirmwareVersion);

	if (_useEntertainmentAPI)
	{
		DebugIf(!_isHueEntertainmentReady, _log, "Bridge is not Entertainment API Ready - Entertainment API usage was disabled!");
		_useEntertainmentAPI = _isHueEntertainmentReady;
	}

	if (isLogging)
	{
		log("Bridge name [ID]", QString("%1 [%2]").arg(_deviceName, getBridgeId()));
		log("Philips Bridge", _isPhilipsHueBridge ? "Yes" : "No");
		log("DIYHue Bridge", _isDiyHue ? "Yes" : "No");
		log("Model", _deviceModel);
		log("Firmware version", _deviceFirmwareVersion);
		log("API-Version", QString("%1.%2.%3").arg(_api_major).arg(_api_minor).arg(_api_patch));
		log("API v2 ready", _isAPIv2Ready ? "Yes" : "No");
		log("Entertainment ready", _isHueEntertainmentReady ? "Yes" : "No");
		log("Use Entertainment API", _useEntertainmentAPI ? "Yes" : "No");
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
		DebugIf(verbose, _log, "jsonLightsInfo: [%s]", QJsonDocument(jsonLightsInfo).toJson(QJsonDocument::Compact).constData());

		// Get all available light ids and their values
		QStringList keys = jsonLightsInfo.keys();

		for (int i = 0; i < keys.count(); ++i)
		{
			QString key = keys.at(i);
			_lightsMap.insert(key, jsonLightsInfo[key].toObject());
		}
	}

	_lightsCount = static_cast<int>(_lightsMap.count());

	if (_lightsCount == 0)
	{
		this->setInError("No light-IDs found at the Philips Hue Bridge");
	}
	else
	{
		log("Lights at Bridge found", _lightsCount);
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
		DebugIf(verbose, _log, "jsonGroupsInfo: [%s]", QJsonDocument(jsonGroupsInfo).toJson(QJsonDocument::Compact).constData());

		// Get all available group ids and their values
		QStringList keys = jsonGroupsInfo.keys();

		auto _groupsCount = keys.size();
		for (int i = 0; i < _groupsCount; ++i)
		{
			_groupsMap.insert(keys.at(i), jsonGroupsInfo.take(keys.at(i)).toObject());
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

QMap<QString, QJsonObject> LedDevicePhilipsHueBridge::getDevicesMap() const
{
	return _devicesMap;
}

QMap<QString, QJsonObject> LedDevicePhilipsHueBridge::getLightMap() const
{
	return _lightsMap;
}

QMap<QString, QJsonObject> LedDevicePhilipsHueBridge::getGroupMap() const
{
	return _groupsMap;
}

QMap<QString, QJsonObject> LedDevicePhilipsHueBridge::getEntertainmentMap() const
{
	return _entertainmentMap;
}

QJsonObject LedDevicePhilipsHueBridge::getDeviceDetails(const QString &deviceId)
{
	DebugIf(verbose, _log, "[%s]", QSTRING_CSTR(deviceId));
	return _devicesMap.value(deviceId);
}

QJsonObject LedDevicePhilipsHueBridge::getLightDetails(const QString &lightId)
{
	DebugIf(verbose, _log, "[%s]", QSTRING_CSTR(lightId));
	return _lightsMap.value(lightId);
}

QJsonDocument LedDevicePhilipsHueBridge::setLightState(const QString &lightId, const QJsonObject &state)
{
	DebugIf(verbose, _log, "[%s] ", QSTRING_CSTR(lightId));
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

QJsonDocument LedDevicePhilipsHueBridge::getGroupDetails(const QString &groupId)
{
	DebugIf(verbose, _log, "[%s]", QSTRING_CSTR(groupId));
	return retrieveGroupDetails(groupId);
}

QString LedDevicePhilipsHueBridge::getGroupName(const QString &groupId) const
{
	QString groupName;
	if (_groupsMap.contains(groupId))
	{
		QJsonObject group = _groupsMap.value(groupId);
		groupName = group.value(API_GROUP_NAME).toString().trimmed().replace("\"", "");
		DebugIf(verbose, _log, "GroupId [%s]: GroupName: %s", QSTRING_CSTR(groupId), QSTRING_CSTR(groupName));
	}
	else
	{
		Error(_log, "Group ID %s does not exist on this bridge", QSTRING_CSTR(groupId));
	}
	return groupName;
}

QStringList LedDevicePhilipsHueBridge::getGroupLights(const QString &groupId) const
{
	if (!_groupsMap.contains(groupId))
	{
		Error(_log, "Group ID [%s] does not exist on this bridge", QSTRING_CSTR(groupId));
		return {};
	}

	QJsonObject group = _groupsMap.value(groupId);
	QString groupName = getGroupName(groupId);
	QString type = group.value(API_GROUP_TYPE).toString();

	if (type != API_GROUP_TYPE_ENTERTAINMENT_V1 && type != API_GROUP_TYPE_ENTERTAINMENT_CONFIGURATION)
	{
		Error(_log, "Group ID (%s)[%s] is not an entertainment group", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId));
		return {};
	}

	QStringList groupLights;
	if (_useApiV2)
	{
		const QJsonArray lightServices = group.value(API_LIGHT_SERVICES).toArray();
		for (const QJsonValue &light : lightServices)
		{
			groupLights.append(light.toObject().value(API_RID).toString());
		}
	}
	else
	{
		groupLights = group.value(API_RESOURCE_LIGHTS).toVariant().toStringList();
	}

	Info(_log, "Entertainment Group \"%s\" [%s] with %d lights found", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId), groupLights.size());
	return groupLights;
}

QJsonDocument LedDevicePhilipsHueBridge::setGroupState(const QString &groupId, bool state)
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
		cmd.insert(API_STREAM, QJsonObject{{API_STREAM_ACTIVE, state}});
	}
	return put(resourcePath, cmd);
}

QJsonObject LedDevicePhilipsHueBridge::getEntertainmentSrvDetails(const QString &deviceId)
{
	DebugIf(verbose, _log, "getEntertainmentSrvDetails [%s]", QSTRING_CSTR(deviceId));

	QJsonObject details;
	for (const QJsonObject &entertainmentSrv : std::as_const(_entertainmentMap))
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

int LedDevicePhilipsHueBridge::getGroupChannelsCount(const QString &groupId) const
{
	int channelsCount{0};
	// search user groupId inside _groupsMap and create light if found
	if (_groupsMap.contains(groupId))
	{
		QJsonObject group = _groupsMap.value(groupId);
		QString groupName = getGroupName(groupId);
		QString type = group.value(API_GROUP_TYPE).toString();
		if (type == API_GROUP_TYPE_ENTERTAINMENT_CONFIGURATION)
		{
			if (_useApiV2)
			{
				QJsonArray channels = group.value(API_CHANNELS).toArray();
				channelsCount = static_cast<int>(channels.size());
			}
			Info(_log, "Entertainment Group \"%s\" [%s] with %d channels found", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId), channelsCount);
		}
		else
		{
			Error(_log, "Group ID (%s)[%s] is not an entertainment group", QSTRING_CSTR(groupName), QSTRING_CSTR(groupId));
		}
	}
	else
	{
		Error(_log, "Group ID [%s] does not exist on this bridge", QSTRING_CSTR(groupId));
	}
	return channelsCount;
}

QJsonDocument LedDevicePhilipsHueBridge::get(const QString &route)
{
	return get(QStringList{route});
}

QJsonDocument LedDevicePhilipsHueBridge::get(const QStringList &routeElements)
{
	_restApi->setPath(routeElements);
	httpResponse response = _restApi->get();
	_restApi->clearPath();

	if (response.error())
	{
		if (routeElements.isEmpty() &&
			(response.getNetworkReplyError() == QNetworkReply::UnknownNetworkError ||
			 response.getNetworkReplyError() == QNetworkReply::ConnectionRefusedError ||
			 response.getNetworkReplyError() == QNetworkReply::RemoteHostClosedError ||
			 response.getNetworkReplyError() == QNetworkReply::OperationCanceledError))
		{
			Warning(_log, "API request (Get): The Hue Bridge is not ready.");
		}
		else
		{
			QString errorReason = QString("API request (Get) failed with error: '%1'").arg(response.getErrorReason());
			this->setInError(errorReason);
		}
		return {};
	}

	if (checkApiError(response.getBody()))
	{
		return response.getBody();
	}

	if (_useApiV2)
	{
		QJsonObject obj = response.getBody().object();
		if (obj.contains(API_DATA))
		{
			return QJsonDocument{obj.value(API_DATA).toArray()};
		}
	}

	return response.getBody();
}

QJsonDocument LedDevicePhilipsHueBridge::put(const QStringList &routeElements, const QJsonObject &content, bool supressError)
{
	_restApi->setPath(routeElements);
	httpResponse response = _restApi->put(content);
	_restApi->clearPath();

	if (response.error())
	{
		QString errorReason = QString("API request (Put) failed with error: '%1'").arg(response.getErrorReason());
		this->setInError(errorReason);
		return response.getBody();
	}

	if (checkApiError(response.getBody(), supressError))
	{
		return response.getBody();
	}

	if (_useApiV2)
	{
		QJsonObject obj = response.getBody().object();
		if (obj.contains(API_DATA))
		{
			return QJsonDocument{obj.value(API_DATA).toArray()};
		}
	}

	return response.getBody();
}

bool LedDevicePhilipsHueBridge::isStreamOwner(const QString &streamOwner) const
{
	bool isOwner{false};

	if (_useApiV2)
	{
		if (streamOwner != "" && streamOwner == _applicationID)
		{
			isOwner = true;
		}
	}
	else
	{
		if (streamOwner != "" && streamOwner == _authToken)
		{
			isOwner = true;
		}
	}
	return isOwner;
}

QJsonArray LedDevicePhilipsHueBridge::discoverSsdp() const
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

QJsonObject LedDevicePhilipsHueBridge::discover(const QJsonObject & /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType);

	QJsonArray deviceList;

#ifdef ENABLE_MDNS
	QString discoveryMethod("mDNS");
	deviceList = MdnsBrowser::getInstance().data()->getServicesDiscoveredJson(
		MdnsServiceRegister::getServiceType(_activeDeviceType),
		MdnsServiceRegister::getServiceNameFilter(_activeDeviceType),
		DEFAULT_DISCOVER_TIMEOUT);
#else
	QString discoveryMethod("ssdp");
	deviceList = discoverSsdp();
#endif

	devicesDiscovered.insert("discoveryMethod", discoveryMethod);
	devicesDiscovered.insert("devices", deviceList);

	Debug(_log, "devicesDiscovered: [%s]", QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact).constData());

	return devicesDiscovered;
}

QJsonObject LedDevicePhilipsHueBridge::getProperties(const QJsonObject &params)
{
	DebugIf(verbose, _log, "params: [%s]", QJsonDocument(params).toJson(QJsonDocument::Compact).constData());

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = params[CONFIG_PORT].toInt();
	_authToken = params[CONFIG_USERNAME].toString("");
	setBridgeId(params[DEV_DATA_BRIDGEID].toString(""));

	Info(_log, "Get properties for %s, bridge-id: [%s], hostname (%s) ", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(getBridgeId()), QSTRING_CSTR(_hostName));

	NetUtils::resolveMdnsHost(_log, _hostName, _apiPort);

	QJsonDocument bridgeDetails = retrieveBridgeDetails();
	if (bridgeDetails.isEmpty())
	{
		Warning(_log, "%s failed to get properties for bridge-id: [%s]. Unable to retrieve required bridge details.", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(getBridgeId()));
		return {};
	}

	setBridgeDetails(bridgeDetails);
	_useApiV2 = _isAPIv2Ready;

	if (!openRestAPI())
	{
		return {};
	}

	QJsonObject properties;
	if (_authToken == API_RESOURCE_CONFIG)
	{
		properties.insert("properties", bridgeDetails.object());
		properties.insert("isEntertainmentReady", _isHueEntertainmentReady);
		properties.insert("isAPIv2Ready", _isAPIv2Ready);
		
		DebugIf(verbose, _log, "properties: [%s]", QJsonDocument(properties).toJson(QJsonDocument::Compact).constData());
		return properties;
	}

	if (_useApiV2)
	{
		configureSsl();
	}

	if (isInError())
	{
		return {};
	}

	setBaseApiEnvironment(_useApiV2);

	QString filter = params["filter"].toString("");
	_restApi->setPath(filter);

	// Perform request
	httpResponse response = _restApi->get();
	if (response.error())
	{
		Warning(_log, "%s getting properties for bridge-id: [%s] failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(getBridgeId()), QSTRING_CSTR(response.getErrorReason()));
	}
	properties.insert("properties", response.getBody().object());
	properties.insert("isEntertainmentReady", _isHueEntertainmentReady);
	properties.insert("isAPIv2Ready", _isAPIv2Ready);
	
	DebugIf(verbose, _log, "properties: [%s]", QJsonDocument(properties).toJson(QJsonDocument::Compact).constData());
	return properties;
}

QJsonObject LedDevicePhilipsHueBridge::addAuthorization(const QJsonObject &params)
{
	Debug(_log, "params: [%s]", QJsonDocument(params).toJson(QJsonDocument::Compact).constData());

	// Generate a new Phillips-Bridge device client/application key
	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = params[CONFIG_PORT].toInt();
	setBridgeId(params[DEV_DATA_BRIDGEID].toString(""));

	Info(_log, "Add authorized user for %s, bridge-id: [%s], hostname (%s) ", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(getBridgeId()), QSTRING_CSTR(_hostName));

	NetUtils::resolveMdnsHost(_log, _hostName, _apiPort);
	QJsonDocument bridgeDetails = retrieveBridgeDetails();
	if (bridgeDetails.isEmpty())
	{
		Warning(_log, "%s failed to generate an authorization/client key for bridge-id: [%s]. Unable to retrieve required bridge details.", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(getBridgeId()));
		return {};
	}

	setBridgeDetails(bridgeDetails);
	_useApiV2 = _isAPIv2Ready;

	if (!openRestAPI())
	{
		return {};
	}

	if (_useApiV2)
	{
		configureSsl();
	}

	if (isInError())
	{
		return {};
	}

	setBaseApiEnvironment(_useApiV2, API_BASE_PATH_V1);

	QJsonObject clientKeyCmd{{"devicetype", "hyperion#" + QHostInfo::localHostName()}, {"generateclientkey", true}};
	_restApi->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	httpResponse response = _restApi->post(clientKeyCmd);
	if (response.error())
	{
		Warning(_log, "%s generation of an authorization/client key for bridge-id: [%s] failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		return {};
	}

	QJsonObject responseBody;
	if (!checkApiError(response.getBody(), false))
	{
		responseBody = response.getBody().array().first().toObject().value("success").toObject();
	}

	return responseBody;
}

void LedDevicePhilipsHueBridge::setBridgeId(const QString &bridgeId)
{
	_deviceBridgeId = bridgeId.toUpper();
}

QString LedDevicePhilipsHueBridge::getBridgeId() const
{
	return _deviceBridgeId;
}

const std::set<QString> PhilipsHueLight::GAMUT_A_MODEL_IDS =
	{"LLC001", "LLC005", "LLC006", "LLC007", "LLC010", "LLC011", "LLC012", "LLC013", "LLC014", "LST001"};
const std::set<QString> PhilipsHueLight::GAMUT_B_MODEL_IDS =
	{"LCT001", "LCT002", "LCT003", "LCT007", "LLM001"};
const std::set<QString> PhilipsHueLight::GAMUT_C_MODEL_IDS =
	{"LCA001", "LCA002", "LCA003", "LCG002", "LCP001", "LCP002", "LCT010", "LCT011", "LCT012", "LCT014", "LCT015", "LCT016", "LCT024", "LCX001", "LCX002", "LLC020", "LST002"};

PhilipsHueLight::PhilipsHueLight(Logger *log, bool useApiV2, const QString &id, const QJsonObject &lightAttributes, int onBlackTimeToPowerOff,
								 int onBlackTimeToPowerOn)
	: _log(log), _isUsingApiV2(useApiV2), _id(id), _on(false), _transitionTime(0), _color({0.0, 0.0, 0.0}), _hasColor(false), _colorBlack({0.0, 0.0, 0.0}), _lastSendColorTime(0), _lastBlackTime(-1), _lastWhiteTime(-1), _blackScreenTriggered(false), _onBlackTimeToPowerOff(onBlackTimeToPowerOff), _onBlackTimeToPowerOn(onBlackTimeToPowerOn)
{
	if (_isUsingApiV2)
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
		_colorSpace.red = {0.704, 0.296};
		_colorSpace.green = {0.2151, 0.7106};
		_colorSpace.blue = {0.138, 0.08};
		_colorBlack = {0.138, 0.08, 0.0};
	}
	else if (_gamutType == "B")
	{
		_colorSpace.red = {0.675, 0.322};
		_colorSpace.green = {0.409, 0.518};
		_colorSpace.blue = {0.167, 0.04};
		_colorBlack = {0.167, 0.04, 0.0};
	}
	else if (_gamutType == "C")
	{
		_colorSpace.red = {0.6915, 0.3083};
		_colorSpace.green = {0.17, 0.7};
		_colorSpace.blue = {0.1532, 0.0475};
		_colorBlack = {0.1532, 0.0475, 0.0};
	}
	else
	{
		_colorSpace.red = {1.0, 0.0};
		_colorSpace.green = {0.0, 1.0};
		_colorSpace.blue = {0.0, 0.0};
		_colorBlack = {0.0, 0.0, 0.0};
	}
}

void PhilipsHueLight::setDeviceDetails(const QJsonObject &details)
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

void PhilipsHueLight::setEntertainmentSrvDetails(const QJsonObject &details)
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

void PhilipsHueLight::saveOriginalState(const QJsonObject &values)
{
	Debug(_log, "Light: %s, id: %s", QSTRING_CSTR(_name), QSTRING_CSTR(_id));
	if (_isUsingApiV2)
	{
		_originalState[API_STATE_ON] = values[API_STATE_ON];

		QJsonValue color = values[API_COLOR];
		_originalState.insert(API_COLOR, QJsonObject{{API_XY_COORDINATES, color[API_XY_COORDINATES]}});

		_originalState[API_GRADIENT] = values[API_GRADIENT];
		return;
	}

	if (_blackScreenTriggered)
	{
		_blackScreenTriggered = false;
		return;
	}
	// Get state object values which are subject to change.
	if (!values[API_STATE].toObject().contains("on"))
	{
		Error(_log, "Got invalid state object from light ID %s", QSTRING_CSTR(_id));
		return;
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
			state[API_BRIGHTNESS].toDouble() / 254.0};
		_originalColor = _color;
		c = QString("{ \"%1\": [%2, %3], \"%4\": %5 }").arg(API_XY_COORDINATES).arg(_originalColor.x, 0, 'd', 4).arg(_originalColor.y, 0, 'd', 4).arg(API_BRIGHTNESS).arg((_originalColor.bri * 254.0), 0, 'd', 4);
		Debug(_log, "Philips original state stored: %s", QSTRING_CSTR(c));
		_transitionTime = values[API_STATE].toObject()[API_TRANSITIONTIME].toInt();
	}
	// Determine the original state.
	_originalState = state;
}

void PhilipsHueLight::setOnOffState(bool on)
{
	Debug(_log, "Light: %s, id: %s -> %s", QSTRING_CSTR(_name), QSTRING_CSTR(_id), on ? "On" : "Off");
	this->_on = on;
}

void PhilipsHueLight::setTransitionTime(int transitionTime)
{
	this->_transitionTime = transitionTime;
}

void PhilipsHueLight::setColor(const CiColor &color)
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

LedDevicePhilipsHue::LedDevicePhilipsHue(const QJsonObject &deviceConfig)
	: LedDevicePhilipsHueBridge(deviceConfig), _switchOffOnBlack(false), _brightnessFactor(1.0), _transitionTime(1), _isInitLeds(false), _lightsCount(0), _blackLightsTimeout(15000), _blackLevel(0.0), _onBlackTimeToPowerOff(100), _onBlackTimeToPowerOn(100), _candyGamma(true), _groupStreamState(false)
{
}

LedDevice *LedDevicePhilipsHue::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePhilipsHue(deviceConfig);
}

bool LedDevicePhilipsHue::init(const QJsonObject &deviceConfig)
{
	bool isInitOK{false};

	if (!verbose)
	{
		verbose = deviceConfig[CONFIG_VERBOSE].toBool(false);
	}

	// Initialise LedDevice configuration and execution environment
	useEntertainmentAPI(deviceConfig[CONFIG_USE_HUE_ENTERTAINMENT_API].toBool(false));

	// Overwrite non supported/required features
	if (deviceConfig["rewriteTime"].toInt(0) > 0)
	{
		InfoIf(!isUsingEntertainmentApi(), _log, "Device Philips Hue does not require rewrites. Refresh time is ignored.");
		_devConfig["rewriteTime"] = 0;
	}

	_switchOffOnBlack = _devConfig[CONFIG_ON_OFF_BLACK].toBool(true);
	_blackLightsTimeout = _devConfig[CONFIG_BLACK_LIGHTS_TIMEOUT].toInt(15000);
	_brightnessFactor = _devConfig[CONFIG_BRIGHTNESSFACTOR].toDouble(1.0);
	_transitionTime = _devConfig[CONFIG_TRANSITIONTIME].toInt(1);
	_isRestoreOrigState = _devConfig[CONFIG_RESTORE_STATE].toBool(true);
	_groupId = _devConfig[CONFIG_groupId].toString();
	_blackLevel = _devConfig["blackLevel"].toDouble(0.0);
	_onBlackTimeToPowerOff = _devConfig["onBlackTimeToPowerOff"].toInt(100);
	_onBlackTimeToPowerOn = _devConfig["onBlackTimeToPowerOn"].toInt(100);
	_candyGamma = _devConfig["candyGamma"].toBool(true);

	if (_blackLevel < 0.0)
	{
		_blackLevel = 0.0;
	}
	if (_blackLevel > 1.0)
	{
		_blackLevel = 1.0;
	}

	if (LedDevicePhilipsHueBridge::init(_devConfig))
	{
		log("Off on Black", _switchOffOnBlack ? "Yes" : "No");
		log("Brightness Factor", _brightnessFactor);
		log("Transition Time", _transitionTime);
		log("Restore Original State", _isRestoreOrigState ? "Yes" : "No");
		log("Use Hue Entertainment API", isUsingEntertainmentApi() ? "Yes" : "No");
		log("Brightness Threshold", _blackLevel);
		log("CandyGamma", _candyGamma ? "Yes" : "No");
		log("Time powering off when black", _onBlackTimeToPowerOff ? "Yes" : "No");
		log("Time powering on when signalled", _onBlackTimeToPowerOn ? "Yes" : "No");

		if (isUsingEntertainmentApi())
		{
			log("Entertainment API Group-ID", _groupId);

			if (_groupId.isEmpty())
			{
				Error(_log, "Disabling usage of Entertainment API - Group-ID is invalid [%s]", QSTRING_CSTR(_groupId));
				useEntertainmentAPI(false);
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
	_lightIds.clear();

	QStringList lights;

	if (isUsingEntertainmentApi() && !_groupId.isEmpty())
	{
		lights = getGroupLights(_groupId);
	}

	if (lights.empty())
	{
		if (isUsingEntertainmentApi())
		{
			useEntertainmentAPI(false);
			Error(_log, "Group-ID [%s] is not usable - Entertainment API usage was disabled!", QSTRING_CSTR(_groupId));
		}
		lights = _devConfig[CONFIG_LIGHTIDS].toVariant().toStringList();
	}

	_lightIds = lights;
	auto configuredLightsCount = static_cast<int>(_lightIds.size());

	if (configuredLightsCount == 0)
	{
		this->setInError("No light-IDs configured");
		return false;
	}

	Debug(_log, "Lights configured: %d", configuredLightsCount);
	if (!updateLights(getLightMap()))
	{
		return false;
	}

	if (isUsingApiV2() && isUsingEntertainmentApi())
	{
		_channelsCount = getGroupChannelsCount(_groupId);

		Debug(_log, "Channels configured: %d", _channelsCount);
		int ledsCount = getLedCount();
		if (ledsCount != _channelsCount)
		{
			QString errorText = QString("Number of hardware LEDs configured [%1] do not match the Entertainment lights' channel number [%2]."
										" Please update your configuration.")
									.arg(ledsCount)
									.arg(_channelsCount);
			setInError(errorText, false);
			return false;
		}
	}

	return true;
}

bool LedDevicePhilipsHue::initLeds()
{
	_isInitLeds = false;

	if (this->isInError() || !setLights())
	{
		return false;
	}

	if (isUsingEntertainmentApi())
	{
		_groupName = getGroupName(_groupId);
		if (_groupName.isEmpty())
		{
			return false;
		}
	}
	else
	{
		// adapt latchTime to count of user lightIds (bridge 10Hz max overall)
		setLatchTime(100 * getLightsCount());
	}

	_isInitLeds = true;
	return true;
}

bool LedDevicePhilipsHue::updateLights(const QMap<QString, QJsonObject> &map)
{
	bool isInitOK = true;

	// search user lightId inside map and create light if found
	_lights.clear();

	if (!_lightIds.empty())
	{
		_lights.reserve(static_cast<size_t>(_lightIds.size()));
		for (const auto &id : std::as_const(_lightIds))
		{
			if (map.contains(id))
			{
				_lights.emplace_back(_log, isUsingApiV2(), id, map.value(id), _onBlackTimeToPowerOff, _onBlackTimeToPowerOn);
			}
			else
			{
				Warning(_log, "Configured light-ID %s is not available at this bridge", QSTRING_CSTR(id));
			}
		}
	}

	auto lightsCount = static_cast<int>(_lights.size());

	setLightsCount(lightsCount);

	if (lightsCount == 0)
	{
		Error(_log, "No usable lights found!");
		isInitOK = false;
	}
	else
	{
		// Populate additional light details
		int i{1};
		for (PhilipsHueLight &light : _lights)
		{
			light.setDeviceDetails(getDeviceDetails(light.getdeviceId()));
			light.setEntertainmentSrvDetails(getEntertainmentSrvDetails(light.getdeviceId()));

			Info(_log, "Light[%d]: \"%s\" [%s], Product: %s, Model: %s, Segments [%d]",
				 i,
				 QSTRING_CSTR(light.getName()),
				 QSTRING_CSTR(light.getId()),
				 QSTRING_CSTR(light.getProduct()),
				 QSTRING_CSTR(light.getModel()),
				 light.getMaxSegments());
			++i;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHue::openStream()
{
	if (this->isInError())
	{
		return false;
	}

	if (getStreamGroupState())
	{
		if (!isStreamOwner(_streamOwner))
		{
			Error(_log, "Group: \"%s\" [%s] is in use and owned by other user: \"%s\" - Entertainment API not usable", QSTRING_CSTR(_groupName), QSTRING_CSTR(_groupId), QSTRING_CSTR(_streamOwner));
			return false;
		}

		Debug(_log, "Group: \"%s\" [%s] is in use, try to stop stream", QSTRING_CSTR(_groupName), QSTRING_CSTR(_groupId));

		if (!stopStream())
		{
			Error(_log, "Group: \"%s\" [%s] couldn't stop by user: \"%s\" - Entertainment API not usable", QSTRING_CSTR(_groupName), QSTRING_CSTR(_groupId), QSTRING_CSTR(_streamOwner));
			return false;
		}

		Debug(_log, "Stream successful stopped");
		restoreState();
	}

	if (!startStream())
	{
		Error(_log, "Philips Hue Entertainment API could not be initialized!");
		return false;
	}

	if (!ProviderUdpSSL::initNetwork())
	{
		Error(_log, "Philips Hue Entertainment API not connected!");
		return false;
	}

	Info(_log, "Philips Hue Entertainment API successful connected! Start Streaming.");
	return true;
}

bool LedDevicePhilipsHue::startStream()
{
	for (int retries = 3; retries > 0; --retries)
	{
		if (setStreamGroupState(true))
		{
			Debug(_log, "The Entertainment stream started successfully");
			return true;
		}

		if (retries > 1)
		{
			Debug(_log, "Start Entertainment stream. Retrying...");
			QThread::msleep(500);
		}
	}

	this->setInError("The Entertainment stream failed to start. Give up.");
	return false;
}

bool LedDevicePhilipsHue::stopStream()
{
	stopConnection();

	int retries = 3;
	bool success = false;
	while (retries-- > 0)
	{
		if (setStreamGroupState(false))
		{
			success = true;
			break;
		}
		Debug(_log, "Stop Entertainment stream. Retrying...");
		QThread::msleep(500);
	}

	bool rc = success;
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
	QJsonDocument doc = getGroupDetails(_groupId);
	DebugIf(verbose, _log, "GroupDetails: [%s]", QJsonDocument(doc).toJson(QJsonDocument::Compact).constData());

	if (this->isInError())
	{
		return false;
	}

	bool streamState{false};
	if (isUsingApiV2())
	{
		QJsonArray groups = doc.array();
		if (groups.isEmpty())
		{
			this->setInError("No Entertainment/Streaming details in Group found");
		}
		else
		{
			QJsonObject group = groups[0].toObject();
			QString streamStaus = group.value(API_STREAM_STATUS).toString();
			if (streamStaus == API_STREAM_ACTIVE)
			{
				streamState = true;
			}
			QJsonObject streamer = group.value(API_STREAM_ACTIVE_V2).toObject();
			_streamOwner = streamer[API_RID].toString();
		}
	}
	else
	{
		QJsonObject obj = doc.object()[API_STREAM].toObject();

		if (obj.isEmpty())
		{
			this->setInError("No Entertainment/Streaming details in Group found");
		}
		else
		{
			streamState = obj.value(API_STREAM_ACTIVE).toBool();
			_streamOwner = obj.value(API_OWNER).toString();
		}
	}

	return streamState;
}

bool LedDevicePhilipsHue::setStreamGroupState(bool state)
{
	QJsonDocument doc = setGroupState(_groupId, state);
	DebugIf(verbose, _log, "StreamGroupState: [%s]", QJsonDocument(doc).toJson(QJsonDocument::Compact).constData());

	if (isUsingApiV2())
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

	QJsonArray response = doc.array();
	if (response.isEmpty())
	{
		_groupStreamState = false;
		return _groupStreamState;
	}

	QJsonObject msg = response.first().toObject();
	if (!msg.contains(API_SUCCESS))
	{
		QString active = state ? API_STREAM_ACTIVE_VALUE_TRUE : API_STREAM_ACTIVE_VALUE_FALSE;
		Warning(_log, "%s", QSTRING_CSTR(QString("Set stream to %1: Neither error nor success contained in Bridge response...").arg(active)));
		_groupStreamState = false;
		return _groupStreamState;
	}

	// Check original Hue response {"success":{"/groups/groupId/stream/active":activeYesNo}}
	QJsonObject success = msg.value(API_SUCCESS).toObject();
	QString valueName = QString(API_STREAM_RESPONSE_FORMAT).arg(API_RESOURCE_GROUPS, _groupId, API_STREAM, API_STREAM_ACTIVE);
	QJsonValue result = success.value(valueName);
	if (result.isUndefined())
	{
		// Workaround
		// Check diyHue response   {"success":{"/groups/groupId/stream":{"active":activeYesNo}}}
		QString diyHueValueName = QString("/%1/%2/%3").arg(API_RESOURCE_GROUPS, _groupId, API_STREAM);
		result = success.value(diyHueValueName).toObject().value(API_STREAM_ACTIVE);
	}

	_groupStreamState = result.toBool();
	return (_groupStreamState == state);
}

void LedDevicePhilipsHue::stop()
{
	LedDevicePhilipsHueBridge::stop();
}

int LedDevicePhilipsHue::open()
{
	int retval = -1;
	if (LedDevicePhilipsHueBridge::open() == 0)
	{
		if (initLeds())
		{
			retval = 0;
		}
	}
	return retval;
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> &ledValues)
{
	// lights will be empty sometimes
	if (_lights.empty())
	{
		return -1;
	}

	// more lights than LEDs, stop always
	if (static_cast<int>(ledValues.size()) < getLightsCount())
	{
		Error(_log, "More light-IDs configured than LEDs, each light-ID requires one LED!");
		return -1;
	}

	int rc{0};
	if (_isOn)
	{
		if (isUsingEntertainmentApi() && _isInitLeds)
		{
			rc = writeStreamData(ledValues);
		}
		else
		{
			rc = writeSingleLights(ledValues);
		}
	}
	return rc;
}

int LedDevicePhilipsHue::writeSingleLights(const std::vector<ColorRgb> &ledValues)
{
	// Iterate through lights and set colors.
	unsigned int idx = 0;
	for (PhilipsHueLight &light : _lights)
	{
		// Get color.
		ColorRgb color = ledValues.at(idx);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		CiColor xy = CiColor::rgbToCiColor(color.red / 255.0, color.green / 255.0, color.blue / 255.0, light.getColorSpace(), _candyGamma);

		bool isBlack = _switchOffOnBlack && xy.bri <= _blackLevel && light.isBlack(true);

		if (isBlack)
		{
			xy.bri = 0;
			xy.x = 0;
			xy.y = 0;

			if (light.getOnOffState())
			{
				if (isUsingEntertainmentApi())
				{
					this->setColor(light, xy);
					this->setOnOffState(light, false);
				}
				else
				{
					setState(light, false, xy);
				}
			}
		}
		else
		{
			bool isWhite = _switchOffOnBlack && xy.bri > _blackLevel && light.isWhite(true);
			if (isWhite || !_switchOffOnBlack)
			{
				if (isWhite && !light.getOnOffState())
				{
					xy.bri = xy.bri / 2;
				}

				if (isUsingEntertainmentApi())
				{
					this->setOnOffState(light, true);
					this->setColor(light, xy);
				}
				else
				{
					this->setState(light, true, xy);
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
		idx++;
	}
	return 0;
}

int LedDevicePhilipsHue::writeStreamData(const std::vector<ColorRgb> &ledValues, bool flush)
{
	QByteArray msg;

	if (isUsingApiV2())
	{
		auto ledsCount = ledValues.size();
		if (ledsCount != _channelsCount)
		{
			QString errorText = QString("Number of LEDs configured via the layout [%1] do not match the Entertainment lights' channel number [%2]."
										" Please update your configuration.")
									.arg(ledsCount)
									.arg(_channelsCount);
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
		msg.append(reinterpret_cast<const char *>(HEADER_V2), sizeof(HEADER_V2));
		msg.append(_groupId.toLocal8Bit());

		auto maxChannels = static_cast<uint8_t>(ledValues.size());

		ColorRgb color;

		for (uint8_t channel = 0; channel < maxChannels; ++channel)
		{
			if (channel < 20) // v2 max 20 channels
			{
				color = static_cast<ColorRgb>(ledValues.at(channel));

				auto R = static_cast<quint16>(color.red << 8);
				auto G = static_cast<quint16>(color.green << 8);
				auto B = static_cast<quint16>(color.blue << 8);

				msg.append(static_cast<char>(channel));
				const uint16_t payload[] = {qToBigEndian<quint16>(R), qToBigEndian<quint16>(G), qToBigEndian<quint16>(B)};
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
		msg.append(reinterpret_cast<const char *>(HEADER), sizeof(HEADER));

		ColorRgb color;

		uint8_t i = 0;
		for (const PhilipsHueLight &light : _lights)
		{
			if (i < 10) // max 10 lights
			{
				auto id = static_cast<uint8_t>(light.getId().toInt());

				color = static_cast<ColorRgb>(ledValues.at(i));
				auto R = static_cast<quint16>(color.red << 8);
				auto G = static_cast<quint16>(color.green << 8);
				auto B = static_cast<quint16>(color.blue << 8);

				msg.append(2, 0x00);
				msg.append(static_cast<char>(id));
				const uint16_t payload[] = {
					qToBigEndian<quint16>(R), qToBigEndian<quint16>(G), qToBigEndian<quint16>(B)};
				msg.append(reinterpret_cast<const char *>(payload), sizeof(payload));
			}
			++i;
		}
	}

	if (verbose3)
	{
		qDebug() << "Msg Hex:" << msg.toHex(':');
	}

	writeBytes(msg, flush);
	return 0;
}

void LedDevicePhilipsHue::setOnOffState(PhilipsHueLight &light, bool on, bool force)
{
	if (light.getOnOffState() != on || force)
	{
		Debug(_log, "id: %s, on: %d", QSTRING_CSTR(light.getId()), on);
		QStringList resourcePath;
		QJsonObject cmd;

		if (isUsingApiV2())
		{
			resourcePath << API_RESOURCE_LIGHT << light.getId();
			cmd.insert(API_STATE_ON, QJsonObject{{API_STATE_ON, on}});
		}
		else
		{
			resourcePath << API_RESOURCE_LIGHTS << light.getId() << API_STATE;
			cmd.insert(API_STATE_ON, on);
		}
		put(resourcePath, cmd);

		if (!isInError())
		{
			light.setOnOffState(on);
		}
	}
}

void LedDevicePhilipsHue::setTransitionTime(PhilipsHueLight &light)
{
	if (light.getTransitionTime() != _transitionTime)
	{
		QStringList resourcePath;
		QJsonObject cmd;

		if (isUsingApiV2())
		{
			resourcePath << API_RESOURCE_LIGHT << light.getId();
			cmd.insert(API_DYNAMICS, QJsonObject{{API_DURATION, _transitionTime}});
		}
		else
		{
			resourcePath << API_RESOURCE_LIGHTS << light.getId() << API_STATE;
			cmd.insert(API_TRANSITIONTIME, _transitionTime);
		}
		put(resourcePath, cmd);

		if (!isInError())
		{
			light.setTransitionTime(_transitionTime);
		}
	}
}

void LedDevicePhilipsHue::setColor(PhilipsHueLight &light, CiColor &color)
{
	this->setColor(light, color, false);
}

void LedDevicePhilipsHue::setColor(PhilipsHueLight &light, CiColor &color, bool force)
{
	if (!light.hasColor() || light.getColor() != color || force)
	{
		if (!isUsingEntertainmentApi())
		{
			QStringList resourcePath;
			QJsonObject cmd;

			if (isUsingApiV2())
			{
				resourcePath << API_RESOURCE_LIGHT << light.getId();
				const double bri = qMin(_brightnessFactor * color.bri * 100, 100.0);
				QJsonObject colorXY;
				colorXY[API_X_COORDINATE] = color.x;
				colorXY[API_Y_COORDINATE] = color.y;
				cmd.insert(API_COLOR, QJsonObject{{API_XY_COORDINATES, colorXY}});
				cmd.insert(API_DIMMING, QJsonObject{{API_BRIGHTNESS, bri}});
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
			put(resourcePath, cmd);
		}
		else
		{
			color.bri = (qMin(1.0, _brightnessFactor * qMax(0.0, color.bri)));
		}

		if (!isInError())
		{
			light.setColor(color);
		}
	}
}

void LedDevicePhilipsHue::setState(PhilipsHueLight &light, bool on, const CiColor &color)
{
	QJsonObject cmd = buildSetStateCommand(light, on, color);

	if (!cmd.isEmpty())
	{
		QStringList resourcePath;
		if (isUsingApiV2())
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
			light.setTransitionTime(_transitionTime);
			light.setColor(color);
			light.setOnOffState(on);
		}
	}
}

QJsonObject LedDevicePhilipsHue::buildSetStateCommand(PhilipsHueLight& light, bool on, const CiColor& color)
{
	QJsonObject cmd;
	bool forceCmd = (light.getOnOffState() != on);

	if (forceCmd)
	{
		if (isUsingApiV2())
		{
			cmd.insert(API_STATE_ON, QJsonObject{ {API_STATE_ON, on} });
		}
		else
		{
			cmd.insert(API_STATE_ON, on);
		}
	}

	if (isUsingEntertainmentApi() || !light.getOnOffState())
	{
		return cmd;
	}

	if (light.getTransitionTime() != _transitionTime)
	{
		if (isUsingApiV2())
		{
			cmd.insert(API_DYNAMICS, QJsonObject{ {API_DURATION, _transitionTime} });
		}
		else
		{
			cmd.insert(API_TRANSITIONTIME, _transitionTime);
		}
	}

	bool colorChanged = !light.hasColor() || light.getColor() != color;
	bool canUpdateColor = !light.isBusy() || forceCmd;

	if (colorChanged && canUpdateColor)
	{
		if (isUsingApiV2())
		{
			const double bri = qMin(_brightnessFactor * color.bri * 100, 100.0);
			QJsonObject colorXY;
			colorXY[API_X_COORDINATE] = color.x;
			colorXY[API_Y_COORDINATE] = color.y;
			cmd.insert(API_COLOR, QJsonObject{ {API_XY_COORDINATES, colorXY} });
			cmd.insert(API_DIMMING, QJsonObject{ {API_BRIGHTNESS, bri} });
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

	return cmd;
}

void LedDevicePhilipsHue::setLightsCount(int lightsCount)
{
	_lightsCount = lightsCount;
}

bool LedDevicePhilipsHue::switchOn()
{
	if (_isOn)
	{
		Debug(_log, "Device %s is already on. Skipping.", QSTRING_CSTR(_activeDeviceType));
		return true;
	}

	if (!_isDeviceReady)
	{
		emit isOnChanged(_isOn);
		return false;
	}

	Info(_log, "Switching device %s ON", QSTRING_CSTR(_activeDeviceType));
	if (!storeState())
	{
		Warning(_log, "Failed switching device %s ON", QSTRING_CSTR(_activeDeviceType));
		emit isOnChanged(_isOn);
		return false;
	}

	if (isUsingEntertainmentApi())
	{
		if (openStream() && startConnection())
		{
			if (!isUsingApiV2() || isDiyHue()) // DiyHue does not auto switch on, if stream starts
			{
				powerOn();
			}
			_isOn = true;
			setRewriteTime(STREAM_REWRITE_TIME.count());
		}
	}
	else
	{
		if (powerOn())
		{
			_isOn = true;
		}
	}

	if (_isOn)
	{
		Info(_log, "Device %s is ON", QSTRING_CSTR(_activeDeviceType));
	}
	else
	{
		Warning(_log, "Failed switching device %s ON", QSTRING_CSTR(_activeDeviceType));
	}

	emit isOnChanged(_isOn);
	return _isOn;
}

bool LedDevicePhilipsHue::switchOff()
{
	if (!_isOn)
	{
		return true;
	}

	if (!_isDeviceInitialised)
	{
		emit isOnChanged(_isOn);
		return false;
	}

	if (!_isDeviceReady)
	{
		emit isOnChanged(_isOn);
		return true;
	}

	bool rc{false};
	if (isUsingEntertainmentApi() && _groupStreamState)
	{
		Info(_log, "Switching device %s OFF", QSTRING_CSTR(_activeDeviceType));
		setRewriteTime(0);
		_isOn = false;

		if (_isRestoreOrigState)
		{
			stopStream();
			rc = restoreState();
		}
		else
		{
			rc = stopStream();
			if (!isUsingApiV2() || isDiyHue()) // DiyHue does not auto switch off, if stream stops
			{
				rc = powerOff();
			}
		}

		if (rc)
		{
			Info(_log, "Device %s is OFF", QSTRING_CSTR(_activeDeviceType));
		}
		else
		{
			Warning(_log, "Failed switching device %s OFF", QSTRING_CSTR(_activeDeviceType));
		}
	}
	else
	{
		Debug(_log, "LedDevicePhilipsHueBridge::switchOff()");
		rc = LedDevicePhilipsHueBridge::switchOff();
	}

	emit isOnChanged(_isOn);
	return rc;
}

bool LedDevicePhilipsHue::powerOn()
{
	bool rc{true};
	if (_isDeviceReady)
	{
		for (PhilipsHueLight &light : _lights)
		{
			setOnOffState(light, true, true);
		}
	}
	return rc;
}

bool LedDevicePhilipsHue::powerOff()
{
	bool rc{true};
	if (_isDeviceReady)
	{
		for (PhilipsHueLight &light : _lights)
		{
			setOnOffState(light, false, true);
		}
	}
	return rc;
}

bool LedDevicePhilipsHue::storeState()
{
	bool rc{true};
	if (_isRestoreOrigState)
	{
		if (!_lightIds.empty())
		{
			for (PhilipsHueLight &light : _lights)
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
	bool rc{true};
	if (_isRestoreOrigState)
	{
		// Restore device's original state
		if (!_lightIds.empty())
		{
			for (const PhilipsHueLight &light : _lights)
			{
				setLightState(light.getId(), light.getOriginalState());
			}
		}
	}
	return rc;
}

void LedDevicePhilipsHue::identify(const QJsonObject &params)
{
	DebugIf(verbose, _log, "params: [%s]", QJsonDocument(params).toJson(QJsonDocument::Compact).constData());

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = params[CONFIG_PORT].toInt();
	_authToken = params[CONFIG_USERNAME].toString("");
	setBridgeId(params[DEV_DATA_BRIDGEID].toString(""));

	QString lighName = params["lightName"].toString();

	Info(_log, "Identify %s, Light: \"%s\" @%s hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(lighName), QSTRING_CSTR(getBridgeId()), QSTRING_CSTR(_hostName));

	NetUtils::resolveMdnsHost(_log, _hostName, _apiPort);

	QJsonDocument bridgeDetails = retrieveBridgeDetails();
	if (bridgeDetails.isEmpty())
	{
		Warning(_log, "%s failed to identify light: \"%s\" @%s hostname (%s). Unable to retrieve required bridge details.", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(lighName), QSTRING_CSTR(getBridgeId()), QSTRING_CSTR(_hostName));
		return;
	}

	setBridgeDetails(bridgeDetails);
	useApiV2(isAPIv2Ready());

	if (isDiyHue()) // DIYHue does not provide v2 Breathe effects, yet -> fall back to v1
	{
		useApiV2(false);
	}

	if (!openRestAPI())
	{
		return;
	}

	bool const useApiV2 = isUsingApiV2();
	if (useApiV2)
	{
		configureSsl();
	}

	if (isInError())
	{
		return;
	}

	setBaseApiEnvironment(useApiV2);

	QStringList resourcepath;
	QJsonObject cmd;
	if (useApiV2)
	{
		QString lightId = params[API_LIGTH_ID].toString();
		resourcepath << API_RESOURCE_LIGHT << lightId;
		cmd.insert(API_ALERT, QJsonObject{{API_ACTION, API_ACTION_BREATHE}});
	}
	else
	{
		bool on{true};
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
