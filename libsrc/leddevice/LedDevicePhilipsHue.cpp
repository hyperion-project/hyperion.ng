// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

// qt includes
#include <QtCore/qmath.h>
#include <QEventLoop>
#include <QNetworkReply>

#include <stdexcept>
#include <set>
#include <cmath>

bool operator ==(CiColor p1, CiColor p2) {
	return (p1.x == p2.x) && (p1.y == p2.y) && (p1.bri == p2.bri);
}

bool operator !=(CiColor p1, CiColor p2) {
	return !(p1 == p2);
}

PhilipsHueLight::PhilipsHueLight(unsigned int id, QString originalState, QString modelId)
	: id(id)
	, originalState(originalState)
{
	// Hue system model ids (http://www.developers.meethue.com/documentation/supported-lights).
	// Light strips, color iris, ...
	const std::set<QString> GAMUT_A_MODEL_IDS = { "LLC001", "LLC005", "LLC006", "LLC007", "LLC010", "LLC011", "LLC012",
			"LLC013", "LLC014", "LST001" };
	// Hue bulbs, spots, ...
	const std::set<QString> GAMUT_B_MODEL_IDS = { "LCT001", "LCT002", "LCT003", "LCT007", "LLM001" };
	// Hue Lightstrip plus, go ...
	const std::set<QString> GAMUT_C_MODEL_IDS = { "LLC020", "LST002" };
	// Find id in the sets and set the appropiate color space.
	if (GAMUT_A_MODEL_IDS.find(modelId) != GAMUT_A_MODEL_IDS.end())
	{
		colorSpace.red = {0.703f, 0.296f};
		colorSpace.green = {0.2151f, 0.7106f};
		colorSpace.blue = {0.138f, 0.08f};
	}
	else if (GAMUT_B_MODEL_IDS.find(modelId) != GAMUT_B_MODEL_IDS.end())
	{
		colorSpace.red = {0.675f, 0.322f};
		colorSpace.green = {0.4091f, 0.518f};
		colorSpace.blue = {0.167f, 0.04f};
	}
	else if (GAMUT_C_MODEL_IDS.find(modelId) != GAMUT_B_MODEL_IDS.end())
	{
		colorSpace.red = {0.675f, 0.322f};
		colorSpace.green = {0.2151f, 0.7106f};
		colorSpace.blue = {0.167f, 0.04f};
	}
	else
	{
		colorSpace.red = {1.0f, 0.0f};
		colorSpace.green = {0.0f, 1.0f};
		colorSpace.blue = {0.0f, 0.0f};
	}
	// Initialize black color.
	black = rgbToCiColor(0.0f, 0.0f, 0.0f);
	// Initialize color with black
	color = {black.x, black.y, black.bri};
}

float PhilipsHueLight::crossProduct(CiColor p1, CiColor p2)
{
	return p1.x * p2.y - p1.y * p2.x;
}

bool PhilipsHueLight::isPointInLampsReach(CiColor p)
{
	CiColor v1 = { colorSpace.green.x - colorSpace.red.x, colorSpace.green.y - colorSpace.red.y };
	CiColor v2 = { colorSpace.blue.x - colorSpace.red.x, colorSpace.blue.y - colorSpace.red.y };
	CiColor q = { p.x - colorSpace.red.x, p.y - colorSpace.red.y };
	float s = crossProduct(q, v2) / crossProduct(v1, v2);
	float t = crossProduct(v1, q) / crossProduct(v1, v2);
	if ((s >= 0.0f) && (t >= 0.0f) && (s + t <= 1.0f))
	{
		return true;
	}
	return false;
}

CiColor PhilipsHueLight::getClosestPointToPoint(CiColor a, CiColor b, CiColor p)
{
	CiColor AP = { p.x - a.x, p.y - a.y };
	CiColor AB = { b.x - a.x, b.y - a.y };
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
	return {a.x + AB.x * t, a.y + AB.y * t};
}

float PhilipsHueLight::getDistanceBetweenTwoPoints(CiColor p1, CiColor p2)
{
	// Horizontal difference.
	float dx = p1.x - p2.x;
	// Vertical difference.
	float dy = p1.y - p2.y;
	// Absolute value.
	return sqrt(dx * dx + dy * dy);
}

CiColor PhilipsHueLight::rgbToCiColor(float red, float green, float blue)
{
	// Apply gamma correction.
	float r = (red > 0.04045f) ? powf((red + 0.055f) / (1.0f + 0.055f), 2.4f) : (red / 12.92f);
	float g = (green > 0.04045f) ? powf((green + 0.055f) / (1.0f + 0.055f), 2.4f) : (green / 12.92f);
	float b = (blue > 0.04045f) ? powf((blue + 0.055f) / (1.0f + 0.055f), 2.4f) : (blue / 12.92f);
	// Convert to XYZ space.
	float X = r * 0.649926f + g * 0.103455f + b * 0.197109f;
	float Y = r * 0.234327f + g * 0.743075f + b * 0.022598f;
	float Z = r * 0.0000000f + g * 0.053077f + b * 1.035763f;
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
	// Brightness is simply Y in the XYZ space.
	CiColor xy = { cx, cy, Y };
	// Check if the given XY value is within the color reach of our lamps.
	if (!isPointInLampsReach(xy))
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

LedDevicePhilipsHue::LedDevicePhilipsHue(const QJsonObject &deviceConfig)
	: LedDevice()
{
	_deviceReady = init(deviceConfig);

	manager = new QNetworkAccessManager();
	timer.setInterval(3000);
	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()), this, SLOT(restoreStates()));
}

LedDevicePhilipsHue::~LedDevicePhilipsHue()
{
	delete manager;
}

bool LedDevicePhilipsHue::init(const QJsonObject &deviceConfig)
{
	LedDevice::init(deviceConfig);

	host = deviceConfig["output"].toString().toStdString().c_str();
	username = deviceConfig["username"].toString("newdeveloper").toStdString().c_str();
	switchOffOnBlack = deviceConfig["switchOffOnBlack"].toBool(true);
	transitiontime = deviceConfig["transitiontime"].toInt(1);
	lightIds.clear();
	QJsonArray lArray = deviceConfig["lightIds"].toArray();
	for(int i = 0; i < lArray.size(); i++)
	{
		lightIds.push_back(lArray[i].toInt());
	}

	return true;
}

LedDevice* LedDevicePhilipsHue::construct(const QJsonObject &deviceConfig)
{
	return new LedDevicePhilipsHue(deviceConfig);
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> & ledValues)
{
	// Save light states if not done before.
	if (!areStatesSaved())
	{
		saveStates((unsigned int) _ledCount);
		switchOn((unsigned int) _ledCount);
	}
	// If there are less states saved than colors given, then maybe something went wrong before.
	if (lights.size() != (unsigned)_ledCount)
	{
		restoreStates();
		return 0;
	}
	// Iterate through colors and set light states.
	unsigned int idx = 0;
	for (const ColorRgb& color : ledValues)
	{
		// Get lamp.
		PhilipsHueLight& lamp = lights.at(idx);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		CiColor xy = lamp.rgbToCiColor(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f);
		// Write color if color has been changed.
		if (xy != lamp.color)
		{
			// From a color to black.
			if (switchOffOnBlack && lamp.color != lamp.black && xy == lamp.black)
			{
				put(getStateRoute(lamp.id), QString("{\"on\": false}"));
			}
			// From black to a color
			else if (switchOffOnBlack && lamp.color == lamp.black && xy != lamp.black)
			{
				// Send adjust color and brightness command in JSON format.
				// We have to set the transition time each time.
				// Send also command to switch the lamp on.
				put(getStateRoute(lamp.id),
						QString("{\"on\": true, \"xy\": [%1, %2], \"bri\": %3, \"transitiontime\": %4}").arg(xy.x).arg(
								xy.y).arg(qRound(xy.bri * 255.0f)).arg(transitiontime));
			}
			// Normal color change.
			else
			{
				// Send adjust color and brightness command in JSON format.
				// We have to set the transition time each time.
				put(getStateRoute(lamp.id),
						QString("{\"xy\": [%1, %2], \"bri\": %3, \"transitiontime\": %4}").arg(xy.x).arg(xy.y).arg(
								qRound(xy.bri * 255.0f)).arg(transitiontime));
			}
		}
		// Remember last color.
		lamp.color = xy;
		// Next light id.
		idx++;
	}
	timer.start();
	return 0;
}

int LedDevicePhilipsHue::switchOff()
{
	timer.stop();
	// If light states have been saved before, ...
	if (areStatesSaved())
	{
		// ... restore them.
		restoreStates();
	}
	return 0;
}

void LedDevicePhilipsHue::put(QString route, QString content)
{
	QString url = getUrl(route);
	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = manager->put(request, content.toLatin1());
	// Connect finished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();
	// Free space.
	reply->deleteLater();
}

QByteArray LedDevicePhilipsHue::get(QString route)
{
	QString url = getUrl(route);
	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = manager->get(request);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();
	// Read all data of the response.
	QByteArray response = reply->readAll();
	// Free space.
	reply->deleteLater();
	// Return response
	return response;
}

QString LedDevicePhilipsHue::getStateRoute(unsigned int lightId)
{
	return QString("lights/%1/state").arg(lightId);
}

QString LedDevicePhilipsHue::getRoute(unsigned int lightId)
{
	return QString("lights/%1").arg(lightId);
}

QString LedDevicePhilipsHue::getUrl(QString route)
{
	return QString("http://%1/api/%2/%3").arg(host).arg(username).arg(route);
}

void LedDevicePhilipsHue::saveStates(unsigned int nLights)
{
	QJsonParseError error;
	QJsonDocument reader;
	QJsonObject json;
	QByteArray response;
	
	// Clear saved lamps.
	lights.clear();
	
	// Read light ids if none have been supplied by the user.
	if (lightIds.size() != nLights)
	{
		lightIds.clear();
		//
		response = get("lights");
		// Use QJsonDocument to parse reponse.
		reader = QJsonDocument::fromJson(response, &error);
		if (error.error != QJsonParseError::NoError)
		{
			throw std::runtime_error(("No lights found at " + getUrl("lights")).toStdString());
		}
		
		json = reader.object();
		
		// Loop over all children.
		for (QJsonObject::iterator it = json.begin(); it != json.end() && lightIds.size() < nLights; it++)
		{
			int lightId = atoi(it.key().toStdString().c_str());
			lightIds.push_back(lightId);
			Debug(_log, "nLights=%d: found light with id %d.",  nLights, lightId);
		}
		// Check if we found enough lights.
		if (lightIds.size() != nLights)
		{
			throw std::runtime_error(("Not enough lights found at " + getUrl("lights")).toStdString());
		}
	}
	// Iterate lights.
	for (unsigned int i = 0; i < nLights; i++)
	{
		// Read the response.
		response = get(getRoute(lightIds.at(i)));
		// Parse JSON.
		reader = QJsonDocument::fromJson(response, &error);
		
		if (error.error != QJsonParseError::NoError)
		{
			// Error occured, break loop.
			Error(_log, "saveStates(nLights=%d): got invalid response from light %s. (error:%s, offset:%d)",
			      nLights, getUrl(getRoute(lightIds.at(i))).toStdString().c_str(), error.errorString().toLocal8Bit().constData(), error.offset );

			break;
		}
		
		json = reader.object();
		
		// Get state object values which are subject to change.
		QJsonObject state;
		if (!json.contains("state"))
		{
			Error(_log, "saveStates(nLights=%d): got no state for light from %s", nLights, getUrl(getRoute(lightIds.at(i))).toStdString().c_str());
			break;
		}
		if (!json["state"].toObject().contains("on"))
		{
			Error(_log, "saveStates(nLights=%d,): got no valid state from light %s", nLights, getUrl(getRoute(lightIds.at(i))).toStdString().c_str());
			break;
		}
		state["on"] = json["state"].toObject()["on"];
		if (json["state"].toObject()["on"].toBool() == true)
		{
			state["xy"] = json["state"].toObject()["xy"];
			state["bri"] = json["state"].toObject()["bri"];
		}
		// Determine the model id.
		QJsonDocument mId(json["modelid"].toObject());
		QString modelId = mId.toJson().trimmed().replace("\"", "");
		QJsonDocument st(state);
		QString originalState = mId.toJson().trimmed();
		// Save state object.
		lights.push_back(PhilipsHueLight(lightIds.at(i), originalState, modelId));
	}
}

void LedDevicePhilipsHue::switchOn(unsigned int nLights)
{
	for (PhilipsHueLight light : lights)
	{
		put(getStateRoute(light.id), "{\"on\": true}");
	}
}

void LedDevicePhilipsHue::restoreStates()
{
	for (PhilipsHueLight light : lights)
	{
		put(getStateRoute(light.id), light.originalState);
	}
	// Clear saved light states.
	lights.clear();
}

bool LedDevicePhilipsHue::areStatesSaved()
{
	return !lights.empty();
}
