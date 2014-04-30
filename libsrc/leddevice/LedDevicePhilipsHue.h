#pragma once

// STL includes
#include <string>

// Qt includes
#include <QString>
#include <QHttp>

// Leddevice includes
#include <leddevice/LedDevice.h>

/**
 * Implementation for the Philips Hue system.
 * 
 * To use set the device to "philipshue".
 * Uses the official Philips Hue API (http://developers.meethue.com).
 * Framegrabber must be limited to 10 Hz / numer of lights to avoid rate limitation by the hue bridge.
 * Create a new API user name "newdeveloper" on the bridge (http://developers.meethue.com/gettingstarted.html)
 */
class LedDevicePhilipsHue: public LedDevice {
public:
	///
	/// Constructs the device.
	///
	/// @param output the ip address of the bridge
	///
	LedDevicePhilipsHue(const std::string& output);

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

	/// Switch the leds off
	virtual int switchOff();

private:
	/// Array to save the light states.
	std::vector<QString> states;
	/// Ip address of the bridge
	QString host;
	/// User name for the API ("newdeveloper")
	QString username;
	/// Qhttp object for sending requests.
	QHttp* http;

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

	/// Restores the status of all lights.
	void restoreStates();

	///
	/// @return true if light states have been saved.
	///
	bool statesSaved();

	///
	/// Converts an RGB color to the Hue xy color space and brightness
	/// https://github.com/PhilipsHue/PhilipsHueSDK-iOS-OSX/blob/master/ApplicationDesignNotes/RGB%20to%20xy%20Color%20conversion.md
	///
	/// @param red the red component in [0, 1]
	///
	/// @param green the green component in [0, 1]
	///
	/// @param blue the blue component in [0, 1]
	///
	/// @param x converted x component
	///
	/// @param y converted y component
	///
	/// @param brightness converted brightness component
	///
	void rgbToXYBrightness(float red, float green, float blue, float& x, float& y, float& brightness);

};
