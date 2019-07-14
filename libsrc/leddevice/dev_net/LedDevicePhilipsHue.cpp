// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

// qt includes
#include <QtCore/qmath.h>
#include <QNetworkReply>

bool operator ==(CiColor p1, CiColor p2)
{
	return (p1.x == p2.x) && (p1.y == p2.y) && (p1.bri == p2.bri);
}

bool operator !=(CiColor p1, CiColor p2)
{
	return !(p1 == p2);
}

CiColor CiColor::rgbToCiColor(float red, float green, float blue, CiColorTriangle colorSpace)
{
	// Apply gamma correction.
	float r = (red > 0.04045f) ? powf((red + 0.055f) / (1.0f + 0.055f), 2.4f) : (red / 12.92f);
	float g = (green > 0.04045f) ? powf((green + 0.055f) / (1.0f + 0.055f), 2.4f) : (green / 12.92f);
	float b = (blue > 0.04045f) ? powf((blue + 0.055f) / (1.0f + 0.055f), 2.4f) : (blue / 12.92f);
	// Convert to XYZ space.
	float X = r * 0.664511f + g * 0.154324f + b * 0.162028f;
	float Y = r * 0.283881f + g * 0.668433f + b * 0.047685f;
	float Z = r * 0.000088f + g * 0.072310f + b * 0.986039f;
	// Convert to x,y space.
	float cx = X / (X + Y + Z);
	float cy = Y / (X + Y + Z);
	if (std::isnan(cx))
	{
		cx = 0.0f;
	}
	if (std::isnan(cy))
	{
		cy = 0.0f;
	}
	// RGB to HSV/B Conversion after gamma correction use V for brightness, not Y from XYZ Space.
	float bri = fmax(fmax(r, g), b);
	CiColor xy =
	{ cx, cy, bri };
	// Check if the given XY value is within the color reach of our lamps.
	if (!isPointInLampsReach(xy, colorSpace))
	{
		// It seems the color is out of reach let's find the closes color we can produce with our lamp and send this XY value out.
		CiColor pAB = getClosestPointToPoint(colorSpace.red, colorSpace.green, xy);
		CiColor pAC = getClosestPointToPoint(colorSpace.blue, colorSpace.red, xy);
		CiColor pBC = getClosestPointToPoint(colorSpace.green, colorSpace.blue, xy);
		// Get the distances per point and see which point is closer to our Point.
		float dAB = getDistanceBetweenTwoPoints(xy, pAB);
		float dAC = getDistanceBetweenTwoPoints(xy, pAC);
		float dBC = getDistanceBetweenTwoPoints(xy, pBC);
		float lowest = dAB;
		CiColor closestPoint = pAB;
		if (dAC < lowest)
		{
			lowest = dAC;
			closestPoint = pAC;
		}
		if (dBC < lowest)
		{
			lowest = dBC;
			closestPoint = pBC;
		}
		// Change the xy value to a value which is within the reach of the lamp.
		xy.x = closestPoint.x;
		xy.y = closestPoint.y;
	}
	return xy;
}

float CiColor::crossProduct(CiColor p1, CiColor p2)
{
	return p1.x * p2.y - p1.y * p2.x;
}

bool CiColor::isPointInLampsReach(CiColor p, CiColorTriangle colorSpace)
{
	CiColor v1 =
	{ colorSpace.green.x - colorSpace.red.x, colorSpace.green.y - colorSpace.red.y };
	CiColor v2 =
	{ colorSpace.blue.x - colorSpace.red.x, colorSpace.blue.y - colorSpace.red.y };
	CiColor q =
	{ p.x - colorSpace.red.x, p.y - colorSpace.red.y };
	float s = crossProduct(q, v2) / crossProduct(v1, v2);
	float t = crossProduct(v1, q) / crossProduct(v1, v2);
	if ((s >= 0.0f) && (t >= 0.0f) && (s + t <= 1.0f))
	{
		return true;
	}
	return false;
}

CiColor CiColor::getClosestPointToPoint(CiColor a, CiColor b, CiColor p)
{
	CiColor AP =
	{ p.x - a.x, p.y - a.y };
	CiColor AB =
	{ b.x - a.x, b.y - a.y };
	float ab2 = AB.x * AB.x + AB.y * AB.y;
	float ap_ab = AP.x * AB.x + AP.y * AB.y;
	float t = ap_ab / ab2;
	if (t < 0.0f)
	{
		t = 0.0f;
	}
	else if (t > 1.0f)
	{
		t = 1.0f;
	}
	return
	{	a.x + AB.x * t, a.y + AB.y * t};
}

float CiColor::getDistanceBetweenTwoPoints(CiColor p1, CiColor p2)
{
	// Horizontal difference.
	float dx = p1.x - p2.x;
	// Vertical difference.
	float dy = p1.y - p2.y;
	// Absolute value.
	return sqrt(dx * dx + dy * dy);
}

PhilipsHueBridge::PhilipsHueBridge(Logger* log, QString host, QString username)
	: QObject()
	, _log(log)
	, host(host)
	, username(username)
{
	// setup reconnection timer
	bTimer.setInterval(5000);
	bTimer.setSingleShot(true);

	connect(&bTimer, &QTimer::timeout, this, &PhilipsHueBridge::bConnect);
	connect(&manager, &QNetworkAccessManager::finished, this, &PhilipsHueBridge::resolveReply);
}

void PhilipsHueBridge::bConnect(void)
{
	if(username.isEmpty() || host.isEmpty())
	{
		Error(_log,"Username or IP Address is empty!");
	}
	else
	{
		QString url = QString("http://%1/api/%2").arg(host).arg(username);
		Debug(_log, "Connect to bridge %s", QSTRING_CSTR(url));

		QNetworkRequest request(url);
		manager.get(request);
	}
}
void PhilipsHueBridge::resolveReply(QNetworkReply* reply)
{
	// TODO use put request also for network error checking with decent threshold
	if(reply->operation() == QNetworkAccessManager::GetOperation)
	{
		if(reply->error() == QNetworkReply::NoError)
		{
			QByteArray response = reply->readAll();
			QJsonParseError error;
			QJsonDocument doc = QJsonDocument::fromJson(response, &error);
			if (error.error != QJsonParseError::NoError)
			{
				Error(_log, "Got invalid response from bridge");
				return;
			}
			// check for authorization
			if(doc.isArray())
			{
				Error(_log, "Authorization failed, username invalid");
				return;
			}

			QJsonObject obj = doc.object()["lights"].toObject();

			if(obj.isEmpty())
			{
				Error(_log, "Bridge has no registered bulbs/stripes");
				return;
			}

			// get all available light ids and their values
			QStringList keys = obj.keys();
			QMap<quint16,QJsonObject> map;
			for (int i = 0; i < keys.size(); ++i)
			{
				map.insert(keys.at(i).toInt(), obj.take(keys.at(i)).toObject());
			}
			emit newLights(map);
		}
		else
		{
			Error(_log,"Network Error: %s", QSTRING_CSTR(reply->errorString()));
			bTimer.start();
		}
	}
	reply->deleteLater();
}

void PhilipsHueBridge::post(QString route, QString content)
{
	//Debug(_log, "Post %s: %s", QSTRING_CSTR(QString("http://IP/api/USR/%1").arg(route)), QSTRING_CSTR(content));

	QNetworkRequest request(QString("http://%1/api/%2/%3").arg(host).arg(username).arg(route));
	manager.put(request, content.toLatin1());
}

const std::set<QString> PhilipsHueLight::GAMUT_A_MODEL_IDS =
{ "LLC001", "LLC005", "LLC006", "LLC007", "LLC010", "LLC011", "LLC012", "LLC013", "LLC014", "LST001" };
const std::set<QString> PhilipsHueLight::GAMUT_B_MODEL_IDS =
{ "LCT001", "LCT002", "LCT003", "LCT007", "LLM001" };
const std::set<QString> PhilipsHueLight::GAMUT_C_MODEL_IDS =
{ "LLC020", "LST002", "LCT011", "LCT012", "LCT010", "LCT014", "LCT015", "LCT016", "LCT024" };

PhilipsHueLight::PhilipsHueLight(Logger* log, PhilipsHueBridge* bridge, unsigned int id, QJsonObject values)
	: _log(log)
	, bridge(bridge)
	, id(id)
{
	// Get state object values which are subject to change.
	if (!values["state"].toObject().contains("on"))
	{
		Error(_log, "Got invalid state object from light ID %d", id);
	}
	QJsonObject state;
	state["on"] = values["state"].toObject()["on"];
	on = false;
	if (values["state"].toObject()["on"].toBool())
	{
		state["xy"] = values["state"].toObject()["xy"];
		state["bri"] = values["state"].toObject()["bri"];
		on = true;

		color = {
					(float) state["xy"].toArray()[0].toDouble(),
					(float) state["xy"].toArray()[1].toDouble(),
					(float) state["bri"].toDouble() / 255.0f
				};
		transitionTime = values["state"].toObject()["transitiontime"].toInt();
	}
	// Determine the model id.
	modelId = values["modelid"].toString().trimmed().replace("\"", "");
	// Determine the original state.
	originalState = QJsonDocument(state).toJson(QJsonDocument::JsonFormat::Compact).trimmed();
	// Find id in the sets and set the appropriate color space.
	if (GAMUT_A_MODEL_IDS.find(modelId) != GAMUT_A_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut A", modelId.toStdString().c_str(), id);
		colorSpace.red =
		{	0.704f, 0.296f};
		colorSpace.green =
		{	0.2151f, 0.7106f};
		colorSpace.blue =
		{	0.138f, 0.08f};
	}
	else if (GAMUT_B_MODEL_IDS.find(modelId) != GAMUT_B_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut B", modelId.toStdString().c_str(), id);
		colorSpace.red =
		{	0.675f, 0.322f};
		colorSpace.green =
		{	0.409f, 0.518f};
		colorSpace.blue =
		{	0.167f, 0.04f};
	}
	else if (GAMUT_C_MODEL_IDS.find(modelId) != GAMUT_C_MODEL_IDS.end())
	{
		Debug(_log, "Recognized model id %s of light ID %d as gamut C", modelId.toStdString().c_str(), id);
		colorSpace.red =
		{	0.6915f, 0.3083f};
		colorSpace.green =
		{	0.17f, 0.7f};
		colorSpace.blue =
		{	0.1532f, 0.0475f};
	}
	else
	{
		Warning(_log, "Did not recognize model id %s of light ID %d", modelId.toStdString().c_str(), id);
		colorSpace.red =
		{	1.0f, 0.0f};
		colorSpace.green =
		{	0.0f, 1.0f};
		colorSpace.blue =
		{	0.0f, 0.0f};
	}

	Info(_log,"Light ID %d created", id);
}

PhilipsHueLight::~PhilipsHueLight()
{
	// Restore the original state.
	set(originalState);
}

void PhilipsHueLight::set(QString state)
{
	bridge->post(QString("lights/%1/state").arg(id), state);
}

void PhilipsHueLight::setOn(bool on)
{
	if (this->on != on)
	{
		QString arg = on ? "true" : "false";
		set(QString("{ \"on\": %1 }").arg(arg));
	}
	this->on = on;
}

void PhilipsHueLight::setTransitionTime(unsigned int transitionTime)
{
	if (this->transitionTime != transitionTime)
	{
		set(QString("{ \"transitiontime\": %1 }").arg(transitionTime));
	}
	this->transitionTime = transitionTime;
}

void PhilipsHueLight::setColor(CiColor color, float brightnessFactor)
{
	if (this->color != color)
	{
		const int bri = qRound(qMin(254.0f, brightnessFactor * qMax(1.0f, color.bri * 254.0f)));
		set(QString("{ \"xy\": [%1, %2], \"bri\": %3 }").arg(color.x, 0, 'f', 4).arg(color.y, 0, 'f', 4).arg(bri));
	}
	this->color = color;
}

CiColor PhilipsHueLight::getColor() const
{
	return color;
}

CiColorTriangle PhilipsHueLight::getColorSpace() const
{
	return colorSpace;
}

LedDevice* LedDevicePhilipsHue::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePhilipsHue(deviceConfig);
}

LedDevicePhilipsHue::LedDevicePhilipsHue(const QJsonObject& deviceConfig)
	: LedDevice(deviceConfig)
	, _bridge(nullptr)
{

}

LedDevicePhilipsHue::~LedDevicePhilipsHue()
{
	switchOff();
	delete _bridge;
}

void LedDevicePhilipsHue::start()
{
	_bridge = new PhilipsHueBridge(_log, _devConfig["output"].toString(), _devConfig["username"].toString());
	_deviceReady = init(_devConfig);

	connect(_bridge, &PhilipsHueBridge::newLights, this, &LedDevicePhilipsHue::newLights);
	connect(this, &LedDevice::enableStateChanged, this, &LedDevicePhilipsHue::stateChanged);
}

bool LedDevicePhilipsHue::init(const QJsonObject &deviceConfig)
{
	switchOffOnBlack = deviceConfig["switchOffOnBlack"].toBool(true);
	brightnessFactor = (float) deviceConfig["brightnessFactor"].toDouble(1.0);
	transitionTime = deviceConfig["transitiontime"].toInt(1);
	QJsonArray lArray = deviceConfig["lightIds"].toArray();

	QJsonObject newDC = deviceConfig;
	if(!lArray.empty())
	{
		for(const auto i : lArray)
		{
			lightIds.push_back(i.toInt());
		}
		// get light info from bridge
		_bridge->bConnect();

		// adapt latchTime to count of user lightIds (bridge 10Hz max overall)
		newDC.insert("latchTime",QJsonValue(100*(int)lightIds.size()));
	}
	else
	{
		Error(_log,"No light ID provided, abort");
	}

	LedDevice::init(newDC);

	return true;
}

void LedDevicePhilipsHue::newLights(QMap<quint16, QJsonObject> map)
{
	if(!lightIds.empty())
	{
		// search user lightid inside map and create light if found
		lights.clear();
		for(const auto id : lightIds)
		{
			if (map.contains(id))
			{
				lights.push_back(PhilipsHueLight(_log, _bridge, id, map.value(id)));
			}
			else
			{
				Error(_log,"Light id %d isn't used on this bridge", id);
			}
		}
	}
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> & ledValues)
{
	// lights will be empty sometimes
	if(lights.empty()) return -1;

	// more lights then leds, stop always
	if(ledValues.size() < lights.size())
	{
		Error(_log,"More LightIDs configured than leds, each LightID requires one led!");
		return -1;
	}

	// Iterate through lights and set colors.
	unsigned int idx = 0;
	for (PhilipsHueLight& light : lights)
	{
		// Get color.
		ColorRgb color = ledValues.at(idx);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		CiColor xy = CiColor::rgbToCiColor(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f,
				light.getColorSpace());

		if (switchOffOnBlack && xy.bri == 0)
		{
			light.setOn(false);
		}
		else
		{
			light.setOn(true);
			// Write color if color has been changed.
			light.setTransitionTime(transitionTime);
			light.setColor(xy, brightnessFactor);
		}

		idx++;
	}

	return 0;
}

void LedDevicePhilipsHue::stateChanged(bool newState)
{
	if(newState)
		_bridge->bConnect();
	else
		lights.clear();
}
