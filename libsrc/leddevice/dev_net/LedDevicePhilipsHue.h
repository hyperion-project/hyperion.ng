#pragma once

// STL includes
#include <set>
#include <string>
#include <stdarg.h>

// Qt includes
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QNetworkReply>
#include <QtCore/qmath.h>
#include <QStringList>

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"
#include "ProviderUdpSSL.h"

/**
 * A XY color point in the color space of the hue system without brightness.
 */
struct XYColor
{
	/// X component.
	double x;
	/// Y component.
	double y;
};

/**
 * Color triangle to define an available color space for the hue lamps.
 */
struct CiColorTriangle
{
	XYColor red, green, blue;
};

/**
 * A color point in the color space of the hue system.
 */
struct CiColor
{
	/// X component.
	double x;
	/// Y component.
	double y;
	/// The brightness.
	double bri;

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
	/// @return color point
	///
	static CiColor rgbToCiColor(double red, double green, double blue, const CiColorTriangle &colorSpace);

	///
	/// @param p the color point to check
	///
	/// @return true if the color point is covered by the lamp color space
	///
	static bool isPointInLampsReach(CiColor p, const CiColorTriangle &colorSpace);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the cross product between p1 and p2
	///
	static double crossProduct(XYColor p1, XYColor p2);

	///
	/// @param a reference point one
	///
	/// @param b reference point two
	///
	/// @param p the point to which the closest point is to be found
	///
	/// @return the closest color point of p to a and b
	///
	static XYColor getClosestPointToPoint(XYColor a, XYColor b, CiColor p);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the distance between the two points
	///
	static double getDistanceBetweenTwoPoints(CiColor p1, XYColor p2);
};

bool operator==(const CiColor& p1, const CiColor& p2);
bool operator!=(const CiColor& p1, const CiColor& p2);

/**
 * Simple class to hold the id, the latest color, the color space and the original state.
 */
class PhilipsHueLight
{

public:
	// Hue system model ids (http://www.developers.meethue.com/documentation/supported-lights).
	// Light strips, color iris, ...
	static const std::set<QString> GAMUT_A_MODEL_IDS;
	// Hue bulbs, spots, ...
	static const std::set<QString> GAMUT_B_MODEL_IDS;
	// Hue Lightstrip plus, go ...
	static const std::set<QString> GAMUT_C_MODEL_IDS;

	///
	/// Constructs the light.
	///
	/// @param log the logger
	/// @param bridge the bridge
	/// @param id the light id
	///
	PhilipsHueLight(Logger* log, unsigned int id, QJsonObject values, unsigned int ledidx);
	~PhilipsHueLight();

	///
	/// @param on
	///
	void setOnOffState(bool on);

	///
	/// @param transitionTime the transition time between colors in multiples of 100 ms
	///
	void setTransitionTime(int transitionTime);

	///
	/// @param color the color to set
	///
	void setColor(const CiColor& color);

	unsigned int getId() const;

	bool getOnOffState() const;
	int getTransitionTime() const;
	CiColor getColor() const;

	///
	/// @return the color space of the light determined by the model id reported by the bridge.
	CiColorTriangle getColorSpace() const;

	QString getOriginalState() const;

private:

	void saveOriginalState(const QJsonObject& values);

	Logger* _log;
	/// light id
	unsigned int _id;
	unsigned int _ledidx;
	bool _on;
	int _transitionTime;
	CiColor _color;
	/// darkes blue color in hue lamp GAMUT = black
	CiColor _colorBlack;
	/// The model id of the hue lamp which is used to determine the color space.
	QString _modelId;
	QString _lightname;
	CiColorTriangle _colorSpace;

	/// The json string of the original state.
	QJsonObject _originalStateJSON;

	QString _originalState;
	CiColor _originalColor;
};

class LedDevicePhilipsHueBridge : public ProviderUdpSSL
{
	Q_OBJECT

public:

	explicit LedDevicePhilipsHueBridge(const QJsonObject &deviceConfig);
	~LedDevicePhilipsHueBridge() override;

	///
	/// @brief Initialise the access to the REST-API wrapper
	///
	/// @param[in] host
	/// @param[in] port
	/// @param[in] authentication token
	///
	/// @return True, if success
	///
	bool initRestAPI(const QString &hostname, int port, const QString &token );

	///
	/// @param route the route of the POST request.
	///
	/// @param content the content of the POST request.
	///
	QJsonDocument post(const QString& route, const QString& content);

	void setLightState(unsigned int lightId = 0, const QString &state = "");

	QMap<quint16,QJsonObject> getLightMap() const;

	QMap<quint16,QJsonObject> getGroupMap() const;

	QString getGroupName(quint16 groupId = 0) const;

	QJsonArray getGroupLights(quint16 groupId = 0) const;



protected:

	///
	/// @brief Initialise the Hue-Bridge configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the Hue-Bridge device and its SSL-connection
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the Hue-Bridge device and its SSL-connection
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Check, if Hue API response indicate error
	///
	/// @param[in] response from Hue-Bridge in JSON-format
	/// return True, Hue Bridge reports error
	///
	bool checkApiError(const QJsonDocument &response );

	///REST-API wrapper
	ProviderRestApi* _restApi;

	/// Ip address of the bridge
	QString _hostname;
	int _apiPort;
	/// User name for the API ("newdeveloper")
	QString _username;

	bool _useHueEntertainmentAPI;

	QJsonDocument getGroupState( unsigned int groupId );
	QJsonDocument setGroupState( unsigned int groupId, bool state);

	bool isStreamOwner(const QString &streamOwner) const;
	bool initMaps();

	void log(const char* msg, const char* type, ...) const;

	const int * getCiphersuites() const override;

private:

	QJsonDocument getAllBridgeInfos();
	void setBridgeConfig( const QJsonDocument &doc );
	void setLightsMap( const QJsonDocument &doc );
	void setGroupMap( const QJsonDocument &doc );

	//Philips Hue Bridge details
	QString _deviceModel;
	QString _deviceFirmwareVersion;
	QString _deviceAPIVersion;

	uint _api_major;
	uint _api_minor;
	uint _api_patch;

	bool _isHueEntertainmentReady;

	QMap<quint16,QJsonObject> _lightsMap;
	QMap<quint16,QJsonObject> _groupsMap;
};

/**
 * Implementation for the Philips Hue system.
 *
 * To use set the device to "philipshue".
 * Uses the official Philips Hue API (http://developers.meethue.com).
 *
 * @author ntim (github), bimsarck (github)
 */
class LedDevicePhilipsHue: public LedDevicePhilipsHueBridge
{
	Q_OBJECT

public:
	///
	/// @brief Constructs LED-device for Philips Hue Lights system
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDevicePhilipsHue(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LED-device
	///
	~LedDevicePhilipsHue();

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Discover devices of this type available (for configuration).
	/// @note Mainly used for network devices. Allows to find devices, e.g. via ssdp, mDNS or cloud ways.
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover() override;

	///
	/// @brief Get the Hue Bridge device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP [:port]",
	///     "user"  : "username",
	///     "filter": "resource to query", root "/" is used, if empty
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the device to identify it.
	///
	/// Used in context of a set of devices of the same type.
	///
	/// @param[in] params Parameters to address device
	///
	void identify(const QJsonObject& params) override;

	///
	/// @brief Get the number of LEDs supported by the device.
	///
	/// @return Number of device's LEDs
	///
	unsigned int getLightsCount() const { return _lightsCount; }

	void setOnOffState(PhilipsHueLight& light, bool on);
	void setTransitionTime(PhilipsHueLight& light);
	void setColor(PhilipsHueLight& light, CiColor& color);
	void setState(PhilipsHueLight& light, bool on, const CiColor& color);

public slots:

	///
	/// @brief Stops the device.
	///
	/// Includes switching-off the device and stopping refreshes.
	///
	void stop() override;

protected:

	///
	/// Initialise the device's configuration
	///
	/// @param deviceConfig Device's configuration in JSON
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb>& ledValues) override;

	///
	/// @brief Switch the LEDs on.
	///
	/// Takes care that the device is opened and powered-on.
	/// Depending on the configuration, the device may store its current state for later restore.
	/// @see powerOn, storeState
	///
	/// @return True if success
	///
	//bool switchOn() override;

	///
	/// @brief Switch the LEDs off.
	///
	/// Takes care that the LEDs and device are switched-off and device is closed.
	/// Depending on the configuration, the device may be powered-off or restored to its previous state.
	/// @see powerOff, restoreState
	///
	/// @return True, if success
	///
	bool switchOff() override;

	///
	/// @brief Power-/turn on the LED-device.
	///
	/// Powers-/Turns on the LED hardware, if supported.
	///
	/// @return True, if success
	///
	bool powerOn() override;

	///
	/// @brief Power-/turn off the LED-device.
	///
	/// Depending on the device's capability, the device is powered-/turned off or
	/// an off state is simulated by writing "Black to LED" (default).
	///
	/// @return True, if success
	///
	bool powerOff() override;

	///
	/// @brief Store the device's original state.
	///
	/// Save the device's state before hyperion color streaming starts allowing to restore state during switchOff().
	///
	/// @return True if success
	///
	bool storeState() override;

	///
	/// @brief Restore the device's original state.
	///
	/// Restore the device's state as before hyperion color streaming started.
	/// This includes the on/off state of the device.
	///
	/// @return True, if success
	///
	bool restoreState() override;

private slots:

	void noSignalTimeout();

private:

	bool initLeds();

	///
	/// @brief Creates new PhilipsHueLight(s) based on user lightid with bridge feedback
	///
	/// @param map Map of lightid/value pairs of bridge
	///
	void newLights(QMap<quint16, QJsonObject> map);

	bool setLights();

	/// creates new PhilipsHueLight(s) based on user lightid with bridge feedback
	///
	/// @param map Map of lightid/value pairs of bridge
	///
	bool updateLights(const QMap<quint16, QJsonObject> &map);

	///
	/// @brief Set the number of LEDs supported by the device.
	///
	/// @rparam[in] Number of device's LEDs
	//
	void setLightsCount( unsigned int lightsCount);

	bool openStream();
	bool getStreamGroupState();
	bool setStreamGroupState(bool state);
	bool startStream();
	bool stopStream();

	void writeStream();
	int writeSingleLights(const std::vector<ColorRgb>& ledValues);

	bool noSignalDetection();

	void stopBlackTimeoutTimer();

	QByteArray prepareStreamData() const;

	///
	bool _switchOffOnBlack;
	/// The brightness factor to multiply on color change.
	double _brightnessFactor;
	/// Transition time in multiples of 100 ms.
	/// The default of the Hue lights is 400 ms, but we may want it snappier.
	int _transitionTime;

	bool _lightStatesRestored;
	bool _isInitLeds;

	/// Array of the light ids.
	std::vector<quint16> _lightIds;
	/// Array to save the lamps.
	std::vector<PhilipsHueLight> _lights;

	unsigned int _lightsCount;
	quint16 _groupId;

	double _brightnessMin;
	double _brightnessMax;

	bool _allLightsBlack;

	QTimer* _blackLightsTimer;
	int _blackLightsTimeout;
	double _brightnessThreshold;

	int _handshake_timeout_min;
	int _handshake_timeout_max;
	int _ssl_read_timeout;

	bool _stopConnection;

	QString _groupName;
	QString _streamOwner;

	int start_retry_left;
	int stop_retry_left;

};
