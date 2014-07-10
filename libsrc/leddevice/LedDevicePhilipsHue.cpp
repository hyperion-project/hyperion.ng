#include <iostream>
// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

// jsoncpp includes
#include <json/json.h>

// qt includes
#include <QtCore/qmath.h>
#include <QUrl>
#include <QHttpRequestHeader>
#include <QEventLoop>

LedDevicePhilipsHue::LedDevicePhilipsHue(const std::string &output) :
	host(output.c_str()), username("newdeveloper") {
	http = new QHttp(host);
/*	timer.setInterval(3000);
	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()), this, SLOT(restoreStates()));*/
}

LedDevicePhilipsHue::~LedDevicePhilipsHue() {
	delete http;
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> &ledValues) {
	// Save light states if not done before.
	if (!statesSaved())
		saveStates(ledValues.size());
	// Iterate through colors and set light states.
	unsigned int lightId = 0;
	for (const ColorRgb &color : ledValues) {
		lightId++;
		// Send only request to the brigde if color changed (prevents DDOS --> 503)
		if (!oldLedValues.empty())
			if(!hasColorChanged(lightId, &color))
				continue;

		float r = color.red / 255.0f;
		float g = color.green / 255.0f;
		float b = color.blue / 255.0f;

		//set color gamut triangle
		if(std::find(hueBulbs.begin(), hueBulbs.end(), modelIds[(lightId - 1)]) != hueBulbs.end()) {
			Red = {0.675f, 0.322f};
			Green = {0.4091f, 0.518f};
			Blue = {0.167f, 0.04f};
		} else if (std::find(livingColors.begin(),
				 livingColors.end(), modelIds[(lightId - 1)]) != livingColors.end()) {
			Red = {0.703f, 0.296f};
			Green = {0.214f, 0.709f};
			Blue = {0.139f, 0.081f};
		} else {
			Red = {1.0f, 0.0f};
			Green = {0.0f, 1.0f};
			Blue = {0.0f, 0.0f};
		}
		// if color equal black, switch off lamp ...
		if (r == 0.0f && g == 0.0f && b == 0.0f) {
			switchLampOff(lightId);
			continue;
		}
		// ... and if lamp off, switch on
		if (!checkOnStatus(states[(lightId - 1)]))
			switchLampOn(lightId);

		float bri;
		CGPoint p = CGPointMake(0, 0);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		rgbToXYBrightness(r, g, b, &p, bri);
		// Send adjust color and brightness command in JSON format.
		put(getStateRoute(lightId),
			 QString("{\"xy\": [%1, %2], \"bri\": %3}").arg(p.x).arg(p.y).arg(qRound(b * 255.0f)));
	}
	oldLedValues = ledValues;
	//timer.start();
	return 0;
}

bool LedDevicePhilipsHue::hasColorChanged(unsigned int lightId, const ColorRgb *color) {
	bool matchFound = true;
	const ColorRgb &tmpOldColor = oldLedValues[(lightId - 1)];
	if ((*color).red == tmpOldColor.red)
		matchFound = false;
	if (!matchFound && (*color).green == tmpOldColor.green)
		matchFound = false;
	else
		matchFound = true;
	if (!matchFound && (*color).blue == tmpOldColor.blue)
		matchFound = false;
	else
		matchFound = true;

	return matchFound;
}

int LedDevicePhilipsHue::switchOff() {
	//timer.stop();
	// If light states have been saved before, ...
	if (statesSaved()) {
		// ... restore them.
		restoreStates();
	}
	return 0;
}

bool LedDevicePhilipsHue::checkOnStatus(QString status) {
	return status.contains("\"on\":true");
}

void LedDevicePhilipsHue::put(QString route, QString content) {
	QString url = QString("/api/%1/%2").arg(username).arg(route);
	QHttpRequestHeader header("PUT", url);
	header.setValue("Host", host);
	header.setValue("Accept-Encoding", "identity");
	header.setValue("Connection", "keep-alive");
	header.setValue("Content-Length", QString("%1").arg(content.size()));
	QEventLoop loop;
	// Connect requestFinished signal to quit slot of the loop.
	loop.connect(http, SIGNAL(requestFinished(int, bool)), SLOT(quit()));
	// Perfrom request
	http->request(header, content.toAscii());
	// Go into the loop until the request is finished.
	loop.exec();
	//std::cout << http->readAll().data() << std::endl;
}

QByteArray LedDevicePhilipsHue::get(QString route) {
	QString url = QString("/api/%1/%2").arg(username).arg(route);
	// Event loop to block until request finished.
	QEventLoop loop;
	// Connect requestFinished signal to quit slot of the loop.
	loop.connect(http, SIGNAL(requestFinished(int, bool)), SLOT(quit()));
	// Perfrom request
	http->get(url);
	// Go into the loop until the request is finished.
	loop.exec();
	// Read all data of the response.
	return http->readAll();
}

QString LedDevicePhilipsHue::getStateRoute(unsigned int lightId) {
	return QString("lights/%1/state").arg(lightId);
}

QString LedDevicePhilipsHue::getRoute(unsigned int lightId) {
	return QString("lights/%1").arg(lightId);
}

void LedDevicePhilipsHue::saveStates(unsigned int nLights) {
	// Clear saved light states.
	states.clear();
	modelIds.clear();
	// Use json parser to parse reponse.
	Json::Reader reader;
	Json::FastWriter writer;
	// Iterate lights.
	for (unsigned int i = 0; i < nLights; i++) {
		// Read the response.
		QByteArray response = get(getRoute(i + 1));
		// Parse JSON.
		Json::Value json;
		if (!reader.parse(QString(response).toStdString(), json)) {
			// Error occured, break loop.
			break;
		}
		// Save state object values which are subject to change.
		Json::Value state(Json::objectValue);
		state["on"] = json["state"]["on"];
		if (json["state"]["on"] == true) {
			state["xy"] = json["state"]["xy"];
			state["bri"] = json["state"]["bri"];
		}
		// Save state object.
		modelIds.push_back(QString(writer.write(json["modelid"]).c_str()).trimmed().replace("\"", ""));
		states.push_back(QString(writer.write(state).c_str()).trimmed());
	}
}

void LedDevicePhilipsHue::switchLampOn(unsigned int lightId) {
	put(getStateRoute(lightId), "{\"on\": true}");
	states[(lightId - 1)].replace("\"on\":false", "\"on\":true");
}

void LedDevicePhilipsHue::switchLampOff(unsigned int lightId) {
	put(getStateRoute(lightId), "{\"on\": false}");
	states[(lightId - 1)].replace("\"on\":true", "\"on\":false");
}

void LedDevicePhilipsHue::restoreStates() {
	unsigned int lightId = 1;
	for (QString state : states) {
		put(getStateRoute(lightId), state);
		lightId++;
	}
	// Clear saved light states.
	states.clear();
	modelIds.clear();
	oldLedValues.clear();
}

bool LedDevicePhilipsHue::statesSaved() {
	return !states.empty();
}

CGPoint LedDevicePhilipsHue::CGPointMake(float x, float y) {
	CGPoint p;
	p.x = x;
	p.y = y;

	return p;
}

float LedDevicePhilipsHue::CrossProduct(CGPoint p1, CGPoint p2) {
	return (p1.x * p2.y - p1.y * p2.x);
}

bool LedDevicePhilipsHue::CheckPointInLampsReach(CGPoint p) {
	CGPoint v1 = CGPointMake(Green.x - Red.x, Green.y - Red.y);
	CGPoint v2 = CGPointMake(Blue.x - Red.x, Blue.y - Red.y);

	CGPoint q = CGPointMake(p.x - Red.x, p.y - Red.y);

	float s = CrossProduct(q, v2) / CrossProduct(v1, v2);
	float t = CrossProduct(v1, q) / CrossProduct(v1, v2);
	if ((s >= 0.0f) && (t >= 0.0f) && (s + t <= 1.0f))
		return true;
	else
		return false;
}

CGPoint LedDevicePhilipsHue::GetClosestPointToPoint(CGPoint A, CGPoint B, CGPoint P) {
	CGPoint AP = CGPointMake(P.x - A.x, P.y - A.y);
	CGPoint AB = CGPointMake(B.x - A.x, B.y - A.y);
	float ab2 = AB.x * AB.x + AB.y * AB.y;
	float ap_ab = AP.x * AB.x + AP.y * AB.y;

	float t = ap_ab / ab2;

	if (t < 0.0f)
		t = 0.0f;
	else if (t > 1.0f)
		t = 1.0f;

	return CGPointMake(A.x + AB.x * t, A.y + AB.y * t);
}

float LedDevicePhilipsHue::GetDistanceBetweenTwoPoints(CGPoint one, CGPoint two) {
	float dx = one.x - two.x; // horizontal difference
	float dy = one.y - two.y; // vertical difference
	float dist = sqrt(dx * dx + dy * dy);

	return dist;
}

void LedDevicePhilipsHue::rgbToXYBrightness(float red, float green, float blue, CGPoint *xyPoint, float &brightness) {
	//Apply gamma correction.
	float r = (red > 0.04045f) ? powf((red + 0.055f) / (1.0f + 0.055f), 2.4f) : (red / 12.92f);
	float g = (green > 0.04045f) ? powf((green + 0.055f) / (1.0f + 0.055f), 2.4f) : (green / 12.92f);
	float b = (blue > 0.04045f) ? powf((blue + 0.055f) / (1.0f + 0.055f), 2.4f) : (blue / 12.92f);
	//Convert to XYZ space.
	float X = r * 0.649926f + g * 0.103455f + b * 0.197109f;
	float Y = r * 0.234327f + g * 0.743075f + b * 0.022598f;
	float Z = r * 0.0000000f + g * 0.053077f + b * 1.035763f;
	//Convert to x,y space.
	float cx = X / (X + Y + Z + 0.0000001f);
	float cy = Y / (X + Y + Z + 0.0000001f);

	if (isnan(cx))
		cx = 0.0f;
	if (isnan(cy))
		cy = 0.0f;

	(*xyPoint).x = cx;
	(*xyPoint).y = cy;

	//Check if the given XY value is within the colourreach of our lamps.
	bool inReachOfLamps = CheckPointInLampsReach(*xyPoint);

	if (!inReachOfLamps) {
		//It seems the colour is out of reach
		//let's find the closes colour we can produce with our lamp and send this XY value out.

		//Find the closest point on each line in the triangle.
		CGPoint pAB = GetClosestPointToPoint(Red, Green, *xyPoint);
		CGPoint pAC = GetClosestPointToPoint(Blue, Red, *xyPoint);
		CGPoint pBC = GetClosestPointToPoint(Green, Blue, *xyPoint);

		//Get the distances per point and see which point is closer to our Point.
		float dAB = GetDistanceBetweenTwoPoints(*xyPoint, pAB);
		float dAC = GetDistanceBetweenTwoPoints(*xyPoint, pAC);
		float dBC = GetDistanceBetweenTwoPoints(*xyPoint, pBC);

		float lowest = dAB;
		CGPoint closestPoint = pAB;

		if (dAC < lowest) {
			lowest = dAC;
			closestPoint = pAC;
		}
		if (dBC < lowest) {
			lowest = dBC;
			closestPoint = pBC;
		}

		//Change the xy value to a value which is within the reach of the lamp.
		(*xyPoint).x = closestPoint.x;
		(*xyPoint).y = closestPoint.y;
	}

	// Brightness is simply Y in the XYZ space.
	brightness = Y;
}
