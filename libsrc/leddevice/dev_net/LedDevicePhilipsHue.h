#pragma once

// STL includes
#include <set>
#include <string>
#include <stdarg.h>

// Qt includes
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtCore/qmath.h>
#include <QStringList>

// LedDevice includes
#include <leddevice/LedDevice.h>
#include "ProviderRestApi.h"
#include "ProviderUdpSSL.h"

//Streaming message header and payload definition
const uint8_t HEADER[] =
	{
		'H', 'u', 'e', 'S', 't', 'r', 'e', 'a', 'm', //protocol
		0x01, 0x00, //version 1.0
		0x01, //sequence number 1
		0x00, 0x00, //Reserved write 0’s
		0x01, //xy Brightness
		0x00, // Reserved, write 0’s
};

const uint8_t PAYLOAD_PER_LIGHT[] =
	{
		0x01, 0x00, 0x06, //light ID
		//color: 16 bpc
		0xff, 0xff,
		0xff, 0xff,
		0xff, 0xff,
		/*
	(message.R >> 8) & 0xff, message.R & 0xff,
	(message.G >> 8) & 0xff, message.G & 0xff,
	(message.B >> 8) & 0xff, message.B & 0xff
	*/
};

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
	static CiColor rgbToCiColor(double red, double green, double blue, const CiColorTriangle& colorSpace, bool candyGamma);

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
	PhilipsHueLight(Logger* log, int id, QJsonObject values, int ledidx,
		int onBlackTimeToPowerOff,
		int onBlackTimeToPowerOn);

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

	int getId() const;

	bool getOnOffState() const;
	int getTransitionTime() const;
	CiColor getColor() const;
	bool hasColor() const;

	///
	/// @return the color space of the light determined by the model id reported by the bridge.
	CiColorTriangle getColorSpace() const;

	void saveOriginalState(const QJsonObject& values);
	QString getOriginalState() const;

	bool isBusy();
	bool isBlack(bool isBlack);
	bool isWhite(bool isWhite);
	void setBlack();
	void blackScreenTriggered();
private:

	Logger* _log;
	/// light id
	int _id;
	int _ledidx;
	bool _on;
	int _transitionTime;
	CiColor _color;
	bool    _hasColor;
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
	qint64 _lastSendColorTime;
	qint64 _lastBlackTime;
	qint64 _lastWhiteTime;
	bool _blackScreenTriggered;
	qint64 _onBlackTimeToPowerOff;
	qint64 _onBlackTimeToPowerOn;
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
	/// @return True, if success
	///
	bool openRestAPI();

	///
	/// @brief Perform a REST-API GET
	///
	/// @param route the route of the GET request.
	///
	/// @return the content of the GET request.
	///
	QJsonDocument get(const QString& route);

	///
	/// @brief Perform a REST-API POST
	///
	/// @param route the route of the POST request.
	/// @param content the content of the POST request.
	///
	QJsonDocument put(const QString& route, const QString& content, bool supressError = false);

	QJsonDocument getLightState( int lightId);
	void setLightState( int lightId = 0, const QString &state = "");

	QMap<int,QJsonObject> getLightMap() const;

	QMap<int,QJsonObject> getGroupMap() const;

	QString getGroupName(int groupId = 0) const;

	QJsonArray getGroupLights(int groupId = 0) const;

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
	/// @param[in] suppressError Treat an error as a warning
	/// 
	/// return True, Hue Bridge reports error
	///
	bool checkApiError(const QJsonDocument& response, bool supressError = false);

	///
	/// @brief Discover devices of this type available (for configuration).
	/// @note Mainly used for network devices. Allows to find devices, e.g. via ssdp, mDNS or cloud ways.
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

	///
	/// @brief Get the Hue Bridge device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	///     "port"  : port
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
	/// @brief Add an authorization/client-key to the Hue Bridge device
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	///     "port"  : port
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the authorization keys
	///
	QJsonObject addAuthorization(const QJsonObject& params) override;

	///REST-API wrapper
	ProviderRestApi* _restApi;
	int _apiPort;
	/// User name for the API ("newdeveloper")
	QString _authToken;

	bool _useHueEntertainmentAPI;

	QJsonDocument getGroupState( int groupId );
	QJsonDocument setGroupState( int groupId, bool state);

	bool isStreamOwner(const QString &streamOwner) const;
	bool initMaps();

	void log(const char* msg, const char* type, ...) const;

	const int * getCiphersuites() const override;

private:

	///
	/// @brief Discover Philips-Hue devices available (for configuration).
	/// Philips-Hue specific ssdp discovery
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonArray discover();

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

	QMap<int,QJsonObject> _lightsMap;
	QMap<int,QJsonObject> _groupsMap;
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
	~LedDevicePhilipsHue() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Send an update to the device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP
	///     "port"  : port
	///     "user"  : "username",
	///     "filter": "resource to query", root "/" is used, if empty
	/// }
	///@endcode
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

	void setOnOffState(PhilipsHueLight& light, bool on, bool force = false);
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
	/// @return True, if success
	///
	bool switchOn() override;

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

private:

	bool initLeds();

	bool setLights();

	/// creates new PhilipsHueLight(s) based on user lightid with bridge feedback
	///
	/// @param map Map of lightid/value pairs of bridge
	///
	bool updateLights(const QMap<int, QJsonObject> &map);

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

	void writeStream(bool flush = false);
	int writeSingleLights(const std::vector<ColorRgb>& ledValues);

	QByteArray prepareStreamData() const;

	///
	bool _switchOffOnBlack;
	/// The brightness factor to multiply on color change.
	double _brightnessFactor;
	/// Transition time in multiples of 100 ms.
	/// The default of the Hue lights is 400 ms, but we may want it snappier.
	int _transitionTime;

	bool _isInitLeds;

	/// Array of the light ids.
	std::vector<int> _lightIds;
	/// Array to save the lamps.
	std::vector<PhilipsHueLight> _lights;

	int _lightsCount;
	int _groupId;

	int _blackLightsTimeout;
	double _blackLevel;
	int _onBlackTimeToPowerOff;
	int	_onBlackTimeToPowerOn;
	bool _candyGamma;

	// TODO: Check what is the correct class
	uint32_t _handshake_timeout_min;
	uint32_t _handshake_timeout_max;
	bool _stopConnection;

	QString _groupName;
	QString _streamOwner;

	qint64 _lastConfirm;
	int	_lastId;
	bool _groupStreamState;
};
