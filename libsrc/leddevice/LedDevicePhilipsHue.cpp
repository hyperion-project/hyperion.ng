// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

// jsoncpp includes
#include <json/json.h>

// qt includes
#include <QtCore/qmath.h>
#include <QUrl>
#include <QHttpRequestHeader>
#include <QEventLoop>

#include <set>

LedDevicePhilipsHue::LedDevicePhilipsHue(const std::string& output) :
		host(output.c_str()), username("newdeveloper") {
	http = new QHttp(host);
	timer.setInterval(3000);
	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()), this, SLOT(restoreStates()));
}

LedDevicePhilipsHue::~LedDevicePhilipsHue() {
	delete http;
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> & ledValues) {
	// Save light states if not done before.
	if (!areStatesSaved()) {
		saveStates((unsigned int) ledValues.size());
		switchOn((unsigned int) ledValues.size());
	}
	// Iterate through colors and set light states.
	unsigned int lightId = 1;
	for (const ColorRgb& color : ledValues) {
		// Find triangle.
		CGTriangle triangle = triangles.at(lightId - 1);
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		CGPoint xy;
		float b;
		rgbToXYBrightness(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, triangle, xy, b);
		// Send adjust color command in JSON format.
		put(getStateRoute(lightId), QString("{\"xy\": [%1, %2]}").arg(xy.x).arg(xy.y));
		// Send brightness color command in JSON format.
		put(getStateRoute(lightId), QString("{\"bri\": %1}").arg(qRound(b * 255.0f)));
		// Next light id.
		lightId++;
	}
	timer.start();
	return 0;
}

int LedDevicePhilipsHue::switchOff() {
	timer.stop();
	// If light states have been saved before, ...
	if (areStatesSaved()) {
		// ... restore them.
		restoreStates();
	}
	return 0;
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

CGTriangle LedDevicePhilipsHue::getTriangle(QString modelId) {
	const std::set<QString> HUE_BULBS_MODEL_IDS = { "LCT001", "LCT002", "LCT003" };
	const std::set<QString> LIVING_COLORS_MODEL_IDS = { "LLC001", "LLC005", "LLC006", "LLC007", "LLC011", "LLC012",
			"LLC013", "LST001" };
	CGTriangle triangle;
	if (HUE_BULBS_MODEL_IDS.find(modelId) != HUE_BULBS_MODEL_IDS.end()) {
		triangle.red = {0.675f, 0.322f};
		triangle.green = {0.4091f, 0.518f};
		triangle.blue = {0.167f, 0.04f};
	} else if (LIVING_COLORS_MODEL_IDS.find(modelId) != LIVING_COLORS_MODEL_IDS.end()) {
		triangle.red = {0.703f, 0.296f};
		triangle.green = {0.214f, 0.709f};
		triangle.blue = {0.139f, 0.081f};
	} else {
		triangle.red = {1.0f, 0.0f};
		triangle.green = {0.0f, 1.0f};
		triangle.blue = {0.0f, 0.0f};
	}
	return triangle;
}

void LedDevicePhilipsHue::saveStates(unsigned int nLights) {
	// Clear saved light states.
	states.clear();
	triangles.clear();
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
		states.push_back(QString(writer.write(state).c_str()).trimmed());
		// Determine triangle.
		QString modelId = QString(writer.write(json["modelid"]).c_str()).trimmed().replace("\"", "");
		triangles.push_back(getTriangle(modelId));
	}
}

void LedDevicePhilipsHue::switchOn(unsigned int nLights) {
	for (unsigned int i = 0; i < nLights; i++) {
		put(getStateRoute(i + 1), "{\"on\": true}");
	}
}

void LedDevicePhilipsHue::restoreStates() {
	unsigned int lightId = 1;
	for (QString state : states) {
		put(getStateRoute(lightId), state);
		lightId++;
	}
	// Clear saved light states.
	states.clear();
	triangles.clear();
}

bool LedDevicePhilipsHue::areStatesSaved() {
	return !states.empty();
}

float LedDevicePhilipsHue::crossProduct(CGPoint p1, CGPoint p2) {
	return p1.x * p2.y - p1.y * p2.x;
}

bool LedDevicePhilipsHue::isPointInLampsReach(CGTriangle triangle, CGPoint p) {
	CGPoint v1 = { triangle.green.x - triangle.red.x, triangle.green.y - triangle.red.y };
	CGPoint v2 = { triangle.blue.x - triangle.red.x, triangle.blue.y - triangle.red.y };
	CGPoint q = { p.x - triangle.red.x, p.y - triangle.red.y };
	float s = crossProduct(q, v2) / crossProduct(v1, v2);
	float t = crossProduct(v1, q) / crossProduct(v1, v2);
	if ((s >= 0.0f) && (t >= 0.0f) && (s + t <= 1.0f)) {
		return true;
	} else {
		return false;
	}
}

CGPoint LedDevicePhilipsHue::getClosestPointToPoint(CGPoint A, CGPoint B, CGPoint P) {
	CGPoint AP = { P.x - A.x, P.y - A.y };
	CGPoint AB = { B.x - A.x, B.y - A.y };
	float ab2 = AB.x * AB.x + AB.y * AB.y;
	float ap_ab = AP.x * AB.x + AP.y * AB.y;
	float t = ap_ab / ab2;
	if (t < 0.0f) {
		t = 0.0f;
	} else if (t > 1.0f) {
		t = 1.0f;
	}
	return {A.x + AB.x * t, A.y + AB.y * t};
}

float LedDevicePhilipsHue::getDistanceBetweenTwoPoints(CGPoint one, CGPoint two) {
	// Horizontal difference.
	float dx = one.x - two.x;
	// Vertical difference.
	float dy = one.y - two.y;
	float dist = sqrt(dx * dx + dy * dy);
	return dist;
}

void LedDevicePhilipsHue::rgbToXYBrightness(float red, float green, float blue, CGTriangle triangle, CGPoint& xyPoint,
		float& brightness) {
	// Apply gamma correction.
	float r = (red > 0.04045f) ? powf((red + 0.055f) / (1.0f + 0.055f), 2.4f) : (red / 12.92f);
	float g = (green > 0.04045f) ? powf((green + 0.055f) / (1.0f + 0.055f), 2.4f) : (green / 12.92f);
	float b = (blue > 0.04045f) ? powf((blue + 0.055f) / (1.0f + 0.055f), 2.4f) : (blue / 12.92f);
	// Convert to XYZ space.
	float X = r * 0.649926f + g * 0.103455f + b * 0.197109f;
	float Y = r * 0.234327f + g * 0.743075f + b * 0.022598f;
	float Z = r * 0.0000000f + g * 0.053077f + b * 1.035763f;
	// Convert to x,y space.
	float cx = X / (X + Y + Z + 0.0000001f);
	float cy = Y / (X + Y + Z + 0.0000001f);
	if (isnan(cx)) {
		cx = 0.0f;
	}
	if (isnan(cy)) {
		cy = 0.0f;
	}
	xyPoint.x = cx;
	xyPoint.y = cy;
	// Check if the given XY value is within the colourreach of our lamps.
	if (!isPointInLampsReach(triangle, xyPoint)) {
		// It seems the colour is out of reach let's find the closes colour we can produce with our lamp and send this XY value out.
		CGPoint pAB = getClosestPointToPoint(triangle.red, triangle.green, xyPoint);
		CGPoint pAC = getClosestPointToPoint(triangle.blue, triangle.red, xyPoint);
		CGPoint pBC = getClosestPointToPoint(triangle.green, triangle.blue, xyPoint);
		// Get the distances per point and see which point is closer to our Point.
		float dAB = getDistanceBetweenTwoPoints(xyPoint, pAB);
		float dAC = getDistanceBetweenTwoPoints(xyPoint, pAC);
		float dBC = getDistanceBetweenTwoPoints(xyPoint, pBC);
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
		// Change the xy value to a value which is within the reach of the lamp.
		xyPoint.x = closestPoint.x;
		xyPoint.y = closestPoint.y;
	}
	// Brightness is simply Y in the XYZ space.
	brightness = Y;
}
