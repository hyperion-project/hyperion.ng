#pragma once

// STL includes
#include <string>

// Qt includes
#include <QObject>
#include <QString>
#include <QHttp>
#include <QTimer>

// Leddevice includes
#include <leddevice/LedDevice.h>

/**
 * A color point in the color space of the hue system.
 */
struct ColorPoint {
	float x;
	float y;
	float bri;
};

bool operator==(ColorPoint p1, ColorPoint p2);
bool operator!=(ColorPoint p1, ColorPoint p2);

/**
 * Color triangle to define an available color space for the hue lamps.
 */
struct ColorTriangle {
	ColorPoint red, green, blue;
};

/**
 * Simple class to hold the id, the latest color, the color space and the original state.
 */
class HueLamp {
public:
	unsigned int id;
	ColorPoint color;
	ColorTriangle colorSpace;
	QString originalState;

	///
	/// Constructs the lamp.
	///
	/// @param id the light id
	///
	/// @param originalState the json string of the original state
	///
	/// @param modelId the model id of the hue lamp which is used to determine the color space
	///
	HueLamp(unsigned int id, QString originalState, QString modelId);
};

/**
 * Implementation for the Philips Hue system.
 *
 * To use set the device to "philipshue".
 * Uses the official Philips Hue API (http://developers.meethue.com).
 * Framegrabber must be limited to 10 Hz / numer of lights to avoid rate limitation by the hue bridge.
 * Create a new API user name "newdeveloper" on the bridge (http://developers.meethue.com/gettingstarted.html)
 *
 * @author ntim (github)
 */
class LedDevicePhilipsHue: public QObject, public LedDevice {
Q_OBJECT
public:
	///
	/// Constructs the device.
	///
	/// @param output the ip address of the bridge
	///
	/// @param switchOffOnBlack kill lights for black
	///
	LedDevicePhilipsHue(const std::string& output, bool switchOffOnBlack);

	///
	/// Destructor of this device
	///
	virtual ~LedDevicePhilipsHue();

	///
	/// Sends the given led-color values via put request to the hue system
	///
	/// @param ledValues The color-value per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Restores the original state of the leds.
	virtual int switchOff();

private slots:
	/// Restores the status of all lights.
	void restoreStates();

private:
	const static ColorPoint BLACK;
	/// Array to save the lamps.
	std::vector<HueLamp> lamps;
	/// Ip address of the bridge
	QString host;
	/// User name for the API ("newdeveloper")
	QString username;
	/// Qhttp object for sending requests.
	QHttp* http;
	/// Use timer to reset lights when we got into "GRABBINGMODE_OFF".
	QTimer timer;
	///
	bool switchOffOnBlack;

	///
	/// Sends a HTTP GET request (blocking).
	///
	/// @param route the URI of the request
	///
	/// @return response of the request
	///
	QByteArray get(QString route);

	///
	/// Sends a HTTP PUT request (non-blocking).
	///
	/// @param route the URI of the request
	///
	/// @param content content of the request
	///
	void put(QString route, QString content);

	///
	/// @param lightId the id of the hue light (starting from 1)
	///
	/// @return the URI of the light state for PUT requests.
	///
	QString getStateRoute(unsigned int lightId);

	///
	/// @param lightId the id of the hue light (starting from 1)
	///
	/// @return the URI of the light for GET requests.
	///
	QString getRoute(unsigned int lightId);

	///
	/// Queries the status of all lights and saves it.
	///
	/// @param nLights the number of lights
	///
	void saveStates(unsigned int nLights);

	///
	/// Switches the leds on.
	///
	/// @param nLights the number of lights
	///
	void switchOn(unsigned int nLights);

	///
	/// @return true if light states have been saved.
	///
	bool areStatesSaved();

	///
	/// Converts an RGB color to the Hue xy color space and brightness.
	/// https://github.com/PhilipsHue/PhilipsHueSDK-iOS-OSX/blob/master/ApplicationDesignNotes/RGB%20to%20xy%20Color%20conversion.md
	///
	/// @param red the red component in [0, 1]
	///
	/// @param green the green component in [0, 1]
	///
	/// @param blue the blue component in [0, 1]
	///
	/// @param lamp the hue lamp instance used for color space checks.
	///
	/// @return color point
	///
	ColorPoint rgbToXYBrightness(float red, float green, float blue, HueLamp lamp);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the cross product between p1 and p2
	///
	float crossProduct(ColorPoint p1, ColorPoint p2);


	///
	/// @param lamp the hue lamp instance
	///
	/// @param p the color point to check
	///
	/// @return true if the color point is covered by the lamp color space
	///
	bool isPointInLampsReach(HueLamp lamp, ColorPoint p);


	///
	/// @param a reference point one
	///
	/// @param b reference point two
	///
	/// @param p the point to which the closest point is to be found
	///
	/// @return the closest color point of p to a and b
	///
	ColorPoint getClosestPointToPoint(ColorPoint a, ColorPoint b, ColorPoint p);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the distance between the two points
	///
	float getDistanceBetweenTwoPoints(ColorPoint p1, ColorPoint p2);

};
