// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

// ssdp discover
#include <ssdp/SSDPDiscover.h>

// Qt includes
#include <QtCore/qmath.h>
#include <QEventLoop>
#include <QNetworkReply>

//
static const bool verbose  = false;
static const bool verbose3 = false;

// Configuration settings
static const char CONFIG_ADDRESS[] = "output";
//static const char CONFIG_PORT[] = "port";
static const char CONFIG_USERNAME[] ="username";
static const char CONFIG_BRIGHTNESSFACTOR [] = "brightnessFactor";
static const char CONFIG_TRANSITIONTIME [] = "transitiontime";
static const char CONFIG_ON_OFF_BLACK [] = "switchOffOnBlack";
static const char CONFIG_LIGHTIDS [] = "lightIds";

// Device Data elements
static const char DEV_DATA_BRIDGEID[] = "bridgeid";
static const char DEV_DATA_MODEL[] = "modelid";
static const char DEV_DATA_NAME[] = "name";
//static const char DEV_DATA_MANUFACTURER[] = "manufacturer";
static const char DEV_DATA_FIRMWAREVERSION[] = "swversion";
static const char DEV_DATA_APIVERSION[] = "apiversion";

// Philips Hue OpenAPI URLs
static const char API_DEFAULT_PORT[] = "80";
static const char API_URL_FORMAT[] = "http://%1:%2/api/%3/%4";
static const char API_ROOT[] = "";
static const char API_STATE[] ="state";
static const char API_CONFIG[] = "config";
static const char API_LIGHTS[] = "lights";

// List of resources
static const char API_XY_COORDINATES[] = "xy";
static const char API_BRIGHTNESS[] = "bri";
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
static const char API_ERROR_TYPE[]="type";

// Phlips Hue ssdp services
static const char SSDP_ID[] = "urn:schemas-upnp-org:device:Basic:1";
const int SSDP_TIMEOUT = 5000; // timout in ms


bool operator ==(const CiColor& p1, const CiColor& p2)
{
	return ((p1.x == p2.x) && (p1.y == p2.y) && (p1.bri == p2.bri));
}

bool operator != (const CiColor& p1, const CiColor& p2)
{
	return !(p1 == p2);
}

CiColor CiColor::rgbToCiColor(double red, double green, double blue, CiColorTriangle colorSpace)
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
			CiColor pAB = getClosestPointToPoint(colorSpace.red, colorSpace.green, xy);
			CiColor pAC = getClosestPointToPoint(colorSpace.blue, colorSpace.red, xy);
			CiColor pBC = getClosestPointToPoint(colorSpace.green, colorSpace.blue, xy);
			// Get the distances per point and see which point is closer to our Point.
			double dAB = getDistanceBetweenTwoPoints(xy, pAB);
			double dAC = getDistanceBetweenTwoPoints(xy, pAC);
			double dBC = getDistanceBetweenTwoPoints(xy, pBC);
			double lowest = dAB;
			CiColor closestPoint = pAB;
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

double CiColor::crossProduct(CiColor p1, CiColor p2)
{
	return p1.x * p2.y - p1.y * p2.x;
}

bool CiColor::isPointInLampsReach(CiColor p, CiColorTriangle colorSpace)
{
	CiColor v1 = { colorSpace.green.x - colorSpace.red.x, colorSpace.green.y - colorSpace.red.y };
	CiColor v2 = { colorSpace.blue.x - colorSpace.red.x, colorSpace.blue.y - colorSpace.red.y };
	CiColor  q = { p.x - colorSpace.red.x, p.y - colorSpace.red.y };
	double s = crossProduct(q, v2) / crossProduct(v1, v2);
	double t = crossProduct(v1, q) / crossProduct(v1, v2);
	if ((s >= 0.0) && (t >= 0.0) && (s + t <= 1.0))
	{
		return true;
	}
	return false;
}

CiColor CiColor::getClosestPointToPoint(CiColor a, CiColor b, CiColor p)
{
	CiColor AP = { p.x - a.x, p.y - a.y };
	CiColor AB = { b.x - a.x, b.y - a.y };
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
	return { a.x + AB.x * t, a.y + AB.y * t };
}

double CiColor::getDistanceBetweenTwoPoints(CiColor p1, CiColor p2)
{
	// Horizontal difference.
	double dx = p1.x - p2.x;
	// Vertical difference.
	double dy = p1.y - p2.y;
	// Absolute value.
	return sqrt(dx * dx + dy * dy);
}

LedDevicePhilipsHueBridge::LedDevicePhilipsHueBridge(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	  ,	_networkmanager (nullptr)
	  , _api_major(0)
	  , _api_minor(0)
	  , _api_patch(0)
	  , _isHueEntertainmentReady (false)
{
	_devConfig = deviceConfig;
	_deviceReady = false;
}

LedDevicePhilipsHueBridge::~LedDevicePhilipsHueBridge()
{
	if (_networkmanager != nullptr)
	{
		delete _networkmanager;
		_networkmanager = nullptr;
	}
}

bool LedDevicePhilipsHueBridge::init(const QJsonObject &deviceConfig)
{
	// Overwrite non supported/required features
	_devConfig["latchTime"]   = 0;
	if (deviceConfig["rewriteTime"].toInt(0) > 0)
	{
		Info (_log, "Device Philips Hue does not require rewrites. Refresh time is ignored.");
		_devConfig["rewriteTime"] = 0;
	}

	DebugIf(verbose, _log, "deviceConfig: [%s]", QString(QJsonDocument(_devConfig).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	bool isInitOK = LedDevice::init(deviceConfig);

	Debug(_log, "DeviceType        : %s", QSTRING_CSTR( this->getActiveDeviceType() ));
	Debug(_log, "LedCount          : %u", this->getLedCount());
	Debug(_log, "ColorOrder        : %s", QSTRING_CSTR( this->getColorOrder() ));
	Debug(_log, "RefreshTime       : %d", _refresh_timer_interval);
	Debug(_log, "LatchTime         : %d", this->getLatchTime());

	if ( isInitOK )
	{
		//Set hostname as per configuration and_defaultHost default port
		QString address = deviceConfig[ CONFIG_ADDRESS ].toString();

		if (! address.isEmpty() )
		{
			QStringList addressparts = address.split(":", QString::SkipEmptyParts);

			_hostname = addressparts[0];
			if ( addressparts.size() > 1)
			{
				_api_port = addressparts[1];
			}
			else
			{
				_api_port   = API_DEFAULT_PORT;
			}
		}
		_username = deviceConfig[ CONFIG_USERNAME ].toString();

		Debug(_log, "Hostname/IP       : %s", QSTRING_CSTR( _hostname ));
		Debug(_log, "Port              : %s", QSTRING_CSTR( _api_port ));
	}
	return isInitOK;
}

int LedDevicePhilipsHueBridge::open( )
{
	return open (_hostname,_api_port, _username );
}

int LedDevicePhilipsHueBridge::open( const QString& hostname, const QString& port, const QString& username )
{
	_deviceInError = false;
	bool isInitOK = true;

	//If host not configured then discover device
	if ( hostname.isEmpty() )
	{
		//Discover Nanoleaf device
		if ( !discoverDevice() )
		{
			this->setInError("No target IP defined nor Philips Hue Bridge was discovered");
			return false;
		}
	}
	else
	{
		_hostname = hostname;
		_api_port = port;
	}
	_username = username;

	//Get Philips Hue Bridge details and configuration
	if ( _networkmanager == nullptr)
	{
		_networkmanager = new QNetworkAccessManager();
	}

	// Read Lights and Light-Ids
	QString url = getUrl(_hostname, _api_port, _username, API_ROOT );
	QJsonDocument doc = getJson( url );

	DebugIf(verbose, _log, "doc: [%s]", QString(QJsonDocument(doc).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	if ( this->isInError() )
	{
		isInitOK = false;
	}
	else
	{

		QJsonObject jsonConfigInfo = doc[API_CONFIG].toObject();
		if ( verbose )
		{
			std::cout <<  "jsonConfigInfo: ]" << QString(QJsonDocument(jsonConfigInfo).toJson(QJsonDocument::Compact)).toUtf8().constData() << "]" << std::endl;
		}

		QString deviceName = jsonConfigInfo[DEV_DATA_NAME].toString();
		_deviceModel = jsonConfigInfo[DEV_DATA_MODEL].toString();
		QString deviceBridgeID = jsonConfigInfo[DEV_DATA_BRIDGEID].toString();
		_deviceFirmwareVersion = jsonConfigInfo[DEV_DATA_FIRMWAREVERSION].toString();
		_deviceAPIVersion = jsonConfigInfo[DEV_DATA_APIVERSION].toString();

		QStringList apiVersionParts = _deviceAPIVersion.split(".", QString::SkipEmptyParts);
		if ( !apiVersionParts.isEmpty())
		{
			_api_major = apiVersionParts[0].toUInt();
			_api_minor = apiVersionParts[1].toUInt();
			_api_patch = apiVersionParts[2].toUInt();

			if ( _api_major > 1 || (_api_major == 1 && _api_minor >= 22) )
			{
				_isHueEntertainmentReady = true;
			}
		}


		Debug(_log, "Bridge Name       : %s", QSTRING_CSTR( deviceName ));
		Debug(_log, "Model             : %s", QSTRING_CSTR( _deviceModel ));
		Debug(_log, "Bridge-ID         : %s", QSTRING_CSTR( deviceBridgeID ));
		Debug(_log, "SoftwareVersion   : %s", QSTRING_CSTR( _deviceFirmwareVersion));
		Debug(_log, "API-Version       : %u.%u.%u", _api_major,_api_minor, _api_patch );
		Debug(_log, "EntertainmentReady: %d", _isHueEntertainmentReady);

		QJsonObject jsonLightsInfo = doc[API_LIGHTS].toObject();
		DebugIf(verbose, _log, "jsonLightsInfo: [%s]", QString(QJsonDocument(jsonLightsInfo).toJson(QJsonDocument::Compact)).toUtf8().constData() );

		// Get all available light ids and their values
		QStringList keys = jsonLightsInfo.keys();

		_ledCount = keys.size();
		for (uint i = 0; i < _ledCount; ++i)
		{
			_lightsMap.insert(keys.at(i).toInt(), jsonLightsInfo.take(keys.at(i)).toObject());
		}

		if (getLightsCount() == 0 )
		{
			setInError("No light-IDs found at the Philips Hue Bridge");
		}
		else
		{
			Debug(_log, "Lights found      : %u", getLightsCount() );
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHueBridge::discoverDevice()
{
	bool isDeviceFound (false);

	// device searching by ssdp
	QString address;
	SSDPDiscover discover;

	// Discover Philips Hue Bridge
	address = discover.getFirstService(STY_WEBSERVER, SSDP_ID, SSDP_TIMEOUT);
	if ( address.isEmpty() )
	{
		Warning(_log, "No Philips Hue Bridge discovered");
	}
	else
	{
		// Philips Hue Bridge found
		Info(_log, "Philips Hue Bridge discovered at [%s]", QSTRING_CSTR( address ));
		isDeviceFound = true;
		QStringList addressparts = address.split(":", QString::SkipEmptyParts);
		_hostname = addressparts[0];
		_api_port = addressparts[1];
	}
	return isDeviceFound;
}

const QMap<quint16,QJsonObject>& LedDevicePhilipsHueBridge::getLightMap(void)
{
	return _lightsMap;
}

QString LedDevicePhilipsHueBridge::getUrl(QString host, QString port, QString auth_token, QString endpoint) const {
	return QString(API_URL_FORMAT).arg(host, port, auth_token, endpoint);
}

QJsonDocument LedDevicePhilipsHueBridge::getJson(QString url)
{

	DebugIf(verbose, _log, "GET: [%s]", QSTRING_CSTR( url ));

	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = _networkmanager->get(request);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();

	QJsonDocument jsonDoc;
	if(reply->operation() == QNetworkAccessManager::GetOperation)
	{
		jsonDoc = handleReply( reply );
	}
	// Free space.
	reply->deleteLater();
	// Return response
	return jsonDoc;
}

QJsonDocument LedDevicePhilipsHueBridge::putJson(QString url, QString json)
{

	DebugIf(verbose, _log, "PUT: [%s] [%s]", QSTRING_CSTR( url ), QSTRING_CSTR( json ) );
	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = _networkmanager->put(request, json.toUtf8());
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();

	QJsonDocument jsonDoc;
	if(reply->operation() == QNetworkAccessManager::PutOperation)
	{
		jsonDoc = handleReply( reply );
	}
	// Free space.
	reply->deleteLater();

	// Return response
	return jsonDoc;
}

QJsonDocument LedDevicePhilipsHueBridge::handleReply(QNetworkReply* const &reply )
{
	QJsonDocument jsonDoc;

	int httpStatusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
	DebugIf(verbose, _log, "Reply.httpStatusCode [%d]", httpStatusCode );
	QString errorReason;

	if(reply->error() == QNetworkReply::NoError)
	{
		if ( httpStatusCode != 204 ){
			QByteArray response = reply->readAll();
			QJsonParseError error;
			jsonDoc = QJsonDocument::fromJson(response, &error);
			if (error.error != QJsonParseError::NoError)
			{
				this->setInError ( "Got invalid response" );
			}
			else
			{
				QString strJson(jsonDoc.toJson(QJsonDocument::Compact));
				DebugIf(verbose, _log, "Reply: [%s]", strJson.toUtf8().constData() );

				QVariantList rspList = jsonDoc.toVariant().toList();
				if ( !rspList.isEmpty() )
				{
					QVariantMap map = rspList.first().toMap();
					if ( map.contains(API_ERROR) )
					{
						// API call failsed to execute an error message was returned
						QString errorAddress = map.value(API_ERROR).toMap().value(API_ERROR_ADDRESS).toString();
						QString errorDesc    = map.value(API_ERROR).toMap().value(API_ERROR_DESCRIPTION).toString();
						QString errorType    = map.value(API_ERROR).toMap().value(API_ERROR_TYPE).toString();

						Debug(_log, "Error Type        : %s", QSTRING_CSTR( errorType ));
						Debug(_log, "Error Address     : %s", QSTRING_CSTR( errorAddress ));
						Debug(_log, "Error Description : %s", QSTRING_CSTR( errorDesc ));

						errorReason = QString ("(%1) %2, Resource:%3").arg(errorType, errorDesc, errorAddress);
						this->setInError ( errorReason );
					}
				}
			}
		}
	}
	else
	{
		if ( httpStatusCode > 0 ) {
			QString httpReason = reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
			QString advise;
			switch ( httpStatusCode ) {
			case 400:
				advise = "Check Request Body";
				break;
			case 401:
				advise = "Check Authentication Token (API Key)";
				break;
			case 404:
				advise = "Check Resource given";
				break;
			default:
				break;
			}
			errorReason = QString ("%1:%2 [%3 %4] - %5").arg(_hostname, _api_port, QString(httpStatusCode) , httpReason, advise);
		}
		else {
			errorReason = QString ("%1:%2 - %3").arg(_hostname, _api_port, reply->errorString());
		}
		this->setInError ( errorReason );
	}
	// Return response
	return jsonDoc;
}

void LedDevicePhilipsHueBridge::post(const QString& route, const QString& content)
{
	QString url = getUrl(_hostname, _api_port, _username, route );
	putJson( url, content );
}

void LedDevicePhilipsHueBridge::setLightState(const unsigned int lightId, QString state)
{
	Debug(_log, "SetLightState [%u]: %s", lightId, QSTRING_CSTR(state));
	post( QString("%1/%2/%3").arg(API_LIGHTS).arg(lightId).arg(API_STATE), state );
}

const std::set<QString> PhilipsHueLight::GAMUT_A_MODEL_IDS =
	{ "LLC001", "LLC005", "LLC006", "LLC007", "LLC010", "LLC011", "LLC012", "LLC013", "LLC014", "LST001" };
const std::set<QString> PhilipsHueLight::GAMUT_B_MODEL_IDS =
	{ "LCT001", "LCT002", "LCT003", "LCT007", "LLM001" };
const std::set<QString> PhilipsHueLight::GAMUT_C_MODEL_IDS =
	{ "LLC020", "LST002", "LCT011", "LCT012", "LCT010", "LCT014", "LCT015", "LCT016", "LCT024" };

PhilipsHueLight::PhilipsHueLight(Logger* log, unsigned int id, QJsonObject values, unsigned int ledidx)
	: _log(log)
	  , _id(id)
	  , _ledidx(ledidx)
	  , _on(false)
	  , _transitionTime(0)
	  , _colorBlack({0.0, 0.0, 0.0})
	  , _modelId (values[API_MODEID].toString().trimmed().replace("\"", ""))
{
	// Find id in the sets and set the appropriate color space.
	if (GAMUT_A_MODEL_IDS.find(_modelId) != GAMUT_A_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut A", QSTRING_CSTR(_modelId), id);
		_colorSpace.red		= {0.704, 0.296};
		_colorSpace.green	= {0.2151, 0.7106};
		_colorSpace.blue	= {0.138, 0.08};
		_colorBlack 		= {0.138, 0.08, 0.0};
	}
	else if (GAMUT_B_MODEL_IDS.find(_modelId) != GAMUT_B_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut B", QSTRING_CSTR(_modelId), id);
		_colorSpace.red 	= {0.675, 0.322};
		_colorSpace.green	= {0.409, 0.518};
		_colorSpace.blue 	= {0.167, 0.04};
		_colorBlack 		= {0.167, 0.04, 0.0};
	}
	else if (GAMUT_C_MODEL_IDS.find(_modelId) != GAMUT_C_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut C", QSTRING_CSTR(_modelId), id);
		_colorSpace.red		= {0.6915, 0.3083};
		_colorSpace.green	= {0.17, 0.7};
		_colorSpace.blue 	= {0.1532, 0.0475};
		_colorBlack 		= {0.1532, 0.0475, 0.0};
	}
	else
	{
		Warning(_log, "Did not recognize model id %s of light ID %d", QSTRING_CSTR(_modelId), id);
		_colorSpace.red 	= {1.0, 0.0};
		_colorSpace.green 	= {0.0, 1.0};
		_colorSpace.blue 	= {0.0, 0.0};
		_colorBlack 		= {0.0, 0.0, 0.0};
	}

	saveOriginalState(values);

	_lightname = values["name"].toString().trimmed().replace("\"", "");
	Info(_log,"Light ID %d (\"%s\", LED index \"%d\") created", id, QSTRING_CSTR(_lightname), ledidx);
}

PhilipsHueLight::~PhilipsHueLight()
{
}

unsigned int PhilipsHueLight::getId() const
{
	return _id;
}

QString PhilipsHueLight::getOriginalState()
{
	return _originalState;
}

void PhilipsHueLight::saveOriginalState(const QJsonObject& values)
{
	// Get state object values which are subject to change.
	if (!values[API_STATE].toObject().contains("on"))
	{
		Error(_log, "Got invalid state object from light ID %d", _id);
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
	  , _switchOffOnBlack (false)
	  , _brightnessFactor(1.0)
	  , _transitionTime (1)
{
	_devConfig = deviceConfig;
	_deviceReady = false;
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
	bool isInitOK = LedDevicePhilipsHueBridge::init(deviceConfig);

	if ( isInitOK )
	{
		// Initiatiale LedDevice configuration and execution environment
		_switchOffOnBlack = _devConfig[CONFIG_ON_OFF_BLACK].toBool(true);
		_brightnessFactor = _devConfig[CONFIG_BRIGHTNESSFACTOR].toDouble(1.0);
		_transitionTime   = _devConfig[CONFIG_TRANSITIONTIME].toInt(1);
		QJsonArray lArray = _devConfig[CONFIG_LIGHTIDS].toArray();

		if(!lArray.empty())
		{
			for(const auto i : lArray)
			{
				_lightIds.push_back(i.toInt());
			}
		}

		uint configuredLightsCount = _lightIds.size();
		Debug(_log, "Off on Black      : %d", _switchOffOnBlack );
		Debug(_log, "Brightness Factor : %f", _brightnessFactor );
		Debug(_log, "Transition Time   : %d", _transitionTime );
		Debug(_log, "Light IDs defined : %d", configuredLightsCount );

		if ( configuredLightsCount == 0)
		{
			setInError("No light-IDs configured");
			isInitOK = false;
		}
	}
	return isInitOK;
}

bool LedDevicePhilipsHue::initLeds()
{
	bool isInitOK = false;

	if ( !isInError() )
	{
		updateLights( getLightMap() );
		// adapt latchTime to count of user lightIds (bridge 10Hz max overall)
		_latchTime_ms = 100 * getLightsCount();
		_devConfig["latchTime"]   = _latchTime_ms;
		Debug(_log, "LatchTime updated to %dms", this->getLatchTime());
		isInitOK = true;
	}

	return isInitOK;
}

void LedDevicePhilipsHue::updateLights(QMap<quint16, QJsonObject> map)
{
	if(!_lightIds.empty())
	{
		// search user lightid inside map and create light if found
		_lights.clear();

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
				Warning(_log,"Configured light-ID %d is not available at this bridge", id);
			}
			ledidx++;
		}
	}
}

int LedDevicePhilipsHue::open()
{
	int retval = -1;
	QString errortext;
	_deviceReady = false;

	// General initialisation and configuration of LedDevice
	if ( init(_devConfig) )
	{
		if  ( LedDevicePhilipsHueBridge::open() )
		// Open/Start LedDevice based on configuration
		{
			if ( initLeds() )
			{
				// Everything is OK -> enable device
				_deviceReady = true;
				setEnable(true);
				retval = 0;
			}
		}
	}
	return retval;
}

void LedDevicePhilipsHue::restoreOriginalState()
{
	if(!_lightIds.empty())
	{
		for (PhilipsHueLight& light : _lights)
		{
			setLightState(light.getId(),light.getOriginalState());
		}
	}
}

void LedDevicePhilipsHue::close()
{
	LedDevicePhilipsHueBridge::close();

	if ( _deviceReady)
	{
		//Restore Philips Hue devices state
		restoreOriginalState();
	}
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> & ledValues)
{
	// lights will be empty sometimes
	if(_lights.empty()) return -1;

	// more lights then leds, stop always
	if(ledValues.size() < _lights.size())
	{
		Error(_log,"More light-IDs configured than leds, each light-ID requires one led!");
		return -1;
	}

	writeSingleLights (ledValues);

	return 0;
}

int LedDevicePhilipsHue::writeSingleLights(const std::vector<ColorRgb>& ledValues)
{
	// Iterate through lights and set colors.
	unsigned int idx = 0;
	for (PhilipsHueLight& light : _lights)
	{
		// Get color.
		ColorRgb color = ledValues.at(idx);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		CiColor xy = CiColor::rgbToCiColor(color.red / 255.0, color.green / 255.0, color.blue / 255.0, light.getColorSpace());

		if (_switchOffOnBlack && xy.bri == 0.0)
		{
			this->setOnOffState(light, false);
		}
		else
		{
			this->setOnOffState(light, true);
			// Write color if color has been changed.
			this->setTransitionTime(light, _transitionTime);
			this->setColor(light, xy, _brightnessFactor);
		}
		idx++;
	}
	return 0;
}

void LedDevicePhilipsHue::setOnOffState(PhilipsHueLight& light, bool on)
{
	if (light.getOnOffState() != on)
	{
		light.setOnOffState( on );
		QString state = on ? API_STATE_VALUE_TRUE : API_STATE_VALUE_FALSE;
		setLightState( light.getId(), QString("{\"%1\": %2 }").arg(API_STATE_ON, state) );
	}
}

void LedDevicePhilipsHue::setTransitionTime(PhilipsHueLight& light, unsigned int transitionTime)
{
	if (light.getTransitionTime() != transitionTime)
	{
		light.setTransitionTime( transitionTime );
		setLightState( light.getId(), QString("{\"%1\": %2 }").arg(API_TRANSITIONTIME).arg( transitionTime ) );
	}
}

void LedDevicePhilipsHue::setColor(PhilipsHueLight& light, const CiColor& color, double brightnessFactor)
{
	const int bri = qRound(qMin(254.0, brightnessFactor * qMax(1.0, color.bri * 254.0)));
	if (light.getColor() != color)
	{
		light.setColor( color) ;
		QString stateCmd = QString("{\"%1\": [%2, %3], \"%4\": %5 }").arg(API_XY_COORDINATES).arg(color.x, 0, 'd', 4).arg(color.y, 0, 'd', 4).arg (API_BRIGHTNESS).arg(bri);
		setLightState( light.getId(), stateCmd );
	}
}

int LedDevicePhilipsHue::switchOn()
{
	if ( _deviceReady)
	{
		//Switch on Philips Hue devices physically
		for (PhilipsHueLight& light : _lights)
		{
			setOnOffState(light,true);
		}
	}
	return 0;
}

int LedDevicePhilipsHue::switchOff()
{
	//Set all LEDs to Black
	int rc = LedDevice::switchOff();

	if ( _deviceReady)
	{
		//Switch off Philips Hue devices physically
		for (PhilipsHueLight& light : _lights)
		{
			setOnOffState(light,false);
		}
	}
	return rc;
}
