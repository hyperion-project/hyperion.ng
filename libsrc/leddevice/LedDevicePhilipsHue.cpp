// Local-Hyperion includes
#include "LedDevicePhilipsHue.h"

#include <iostream>

#include <QtCore/qmath.h>
#include <QUrl>
#include <QHttpRequestHeader>
#include <QThread>

LedDevicePhilipsHue::LedDevicePhilipsHue(const std::string& output) :
	host(output.c_str()), username("newdeveloper") {
	http = new QHttp(host, 80);
}

LedDevicePhilipsHue::~LedDevicePhilipsHue() {
	delete http;
}

int LedDevicePhilipsHue::write(const std::vector<ColorRgb> & ledValues) {
	unsigned int lightId = 1;
	for (const ColorRgb& color : ledValues) {
		float x, y, b;
		// Scale colors from [0, 255] to [0, 1] and convert to xy space.
		rgbToXYBrightness(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, x, y, b);
		// Send adjust color command in JSON format.
		put(getRoute(lightId), QString("{\"xy\": [%1, %2]}").arg(x).arg(y));
		// Send brightness color command in JSON format.
		put(getRoute(lightId), QString("{\"bri\": %1}").arg(qRound(b * 255.0f)));
		// Next light id.
		lightId++;
	}
	return 0;
}

int LedDevicePhilipsHue::switchOff() {
	return 0;
}

void LedDevicePhilipsHue::put(QString route, QString content) {
	QString url = QString("/api/%1/%2").arg(username).arg(route);
	QHttpRequestHeader header("PUT", url);
	header.setValue("Host", host);
	header.setValue("Accept-Encoding", "identity");
	header.setValue("Content-Length", QString("%1").arg(content.size()));
	http->setHost(host);
	http->request(header, content.toAscii());
}

QString LedDevicePhilipsHue::getRoute(unsigned int lightId) {
	return QString("lights/%1/state").arg(lightId);
}

void LedDevicePhilipsHue::rgbToXYBrightness(float red, float green, float blue, float& x, float& y, float& brightness) {
	// Apply gamma correction.
	red = (red > 0.04045f) ? qPow((red + 0.055f) / (1.0f + 0.055f), 2.4f) : (red / 12.92f);
	green = (green > 0.04045f) ? qPow ((green + 0.055f) / (1.0f + 0.055f), 2.4f) : (green / 12.92f);
	blue = (blue > 0.04045f) ? qPow ((blue + 0.055f) / (1.0f + 0.055f), 2.4f) : (blue / 12.92f);
	// Convert to XYZ space.
	float X = red * 0.649926f + green * 0.103455f + blue * 0.197109f;
	float Y = red * 0.234327f + green * 0.743075f + blue * 0.022598f;
	float Z = red * 0.0000000f + green * 0.053077f + blue * 1.035763f;
	// Convert to x,y space.
	x = X / (X + Y + Z);
	y = Y / (X + Y + Z);
	if (isnan(x)) {
		x = 0.0f;
	}
	if (isnan(y)) {
		y = 0.0f;
	}
	// Brightness is simply Y in the XYZ space.
	brightness = Y;
}
