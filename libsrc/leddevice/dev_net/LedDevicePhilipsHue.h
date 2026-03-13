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
#include <QScopedPointer>
#include <QJsonObject>
#include <QByteArray>
#include <QVector>
#include <QHostAddress>

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
	XYColor red;
	XYColor green;
	XYColor blue;
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
	/// @param useApiV2 make use of Hue API version 2
	/// @param id the light id
	/// @param lightAttributes the light's attributes as provied by the Hue Bridge
	/// @param onBlackTimeToPowerOff Timeframe of Black output that triggers powering off the light
	/// @param onBlackTimeToPowerOn Timeframe of non Black output that triggers powering on the light
	///
	PhilipsHueLight(QSharedPointer<Logger> log, bool useApiV2, const QString& id, const QJsonObject& lightAttributes,
					int onBlackTimeToPowerOff,
					int onBlackTimeToPowerOn);

	void setDeviceDetails(const QJsonObject& details);
	void setEntertainmentSrvDetails(const QJsonObject& details);

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

	QString getId() const;
	QString getdeviceId() const;
	QString getProduct() const;
	QString getModel() const;
	QString getName() const;
	QString getArcheType() const;

	int getMaxSegments() const;

	bool getOnOffState() const;
	int getTransitionTime() const;
	CiColor getColor() const;
	bool hasColor() const;

	///
	/// @return the color space of the light determined by the model id reported by the bridge.
	CiColorTriangle getColorSpace() const;

	void saveOriginalState(const QJsonObject& values);
	QJsonObject getOriginalState() const;

	bool isBusy();
	bool isBlack(bool isBlack);
	bool isWhite(bool isWhite);
	void setBlack();
	void blackScreenTriggered();

private:

	QSharedPointer<Logger> _log;
	bool _isUsingApiV2;

	QString _id;
	QString _deviceId;
	QString _product;
	QString _model;
	QString _name;
	QString _archeType;
	QString _gamutType;

	int _maxSegments;

	bool _on;
	int _transitionTime;
	CiColor _color;
	bool    _hasColor;
	/// darkes blue color in hue lamp GAMUT = black
	CiColor _colorBlack;
	CiColorTriangle _colorSpace;

	/// The json string of the original state.
	QJsonObject _originalStateJSON;

	QJsonObject _originalState;
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
	/// @brief Perform a REST-API GET
	///
	/// @param routeElements the route's elements of the GET request.
	///
	/// @return the content of the GET request.
	///
	QJsonDocument get(const QStringList& routeElements);

	///
	/// @brief Perform a REST-API PUT
	///
	/// @param routeElements the route's elements of the PUT request.
	/// @param content the content of the PUT request.
	/// @param supressError Treat an error as a warning
	///
	/// @return the content of the PUT request.
	///
	QJsonDocument put(const QStringList& routeElements, const QJsonObject& content, bool supressError = false);

	QJsonDocument retrieveBridgeDetails();
	QJsonObject getDeviceDetails(const QString& deviceId) const;
	QJsonObject getEntertainmentSrvDetails(const QString& deviceId) const;

	QJsonObject getLightDetails(const QString& lightId) const;
	QJsonDocument setLightState(const QString& lightId, const QJsonObject& state);

	QMap<QString,QJsonObject> getDevicesMap() const;
	QMap<QString,QJsonObject> getLightMap() const;
	QMap<QString,QJsonObject> getGroupMap() const;
	QMap<QString,QJsonObject> getEntertainmentMap() const;

	QString getGroupName(const QString& groupId) const;
	QStringList getGroupLights(const QString& groupId) const;
	int getGroupChannelsCount(const QString& groupId) const;

	bool isPhilipsHueBridge() const;
	bool isDiyHue() const;
	bool isAPIv2Ready() const;
	bool isAPIv2Ready(int swVersion) const;
	bool isApiEntertainmentReady(const QString& apiVersion);

	QString getBridgeId() const;
	int getFirmwareVersion() const;

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

	void setBridgeId(const QString& bridgeId);

	void useApiV2(bool useApiV2);
	bool isUsingApiV2() const;

	void useEntertainmentAPI(bool useEntertainmentAPI);
	bool isUsingEntertainmentApi() const;

	void setBridgeDetails( const QJsonDocument &doc, bool isLogging = false );

	void setBaseApiEnvironment(bool apiV2 = true, const QString& path = "");

	QJsonDocument getGroupDetails( const QString& groupId );
	QJsonDocument setGroupState( const QString& groupId, bool state);

	bool isStreamOwner(const QString &streamOwner) const;

	bool initDevicesMap();
	bool initLightsMap();
	bool initGroupsMap();
	bool initEntertainmentSrvsMap();

	void log(const QString& msg, const QVariant& value) const;

	bool configureSsl();
	const int * getCiphersuites() const override;

	///REST-API wrapper
	QScopedPointer<ProviderRestApi> _restApi;
	int _apiPort;
	/// User name for the API ("newdeveloper")
	QString _authToken;

private:

	///
	/// @brief Discover Philips-Hue devices available (for configuration).
	/// Philips-Hue specific ssdp discovery
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonArray discoverSsdp() const;

	QJsonDocument retrieveDeviceDetails(const QString& deviceId = "");
	QJsonDocument retrieveLightDetails(const QString& lightId = "");
	QJsonDocument retrieveGroupDetails(const QString& groupId = "");
	QJsonDocument retrieveEntertainmentSrvDetails(const QString& deviceId = "");

	bool retrieveApplicationId();

	bool handleV1ApiError(const QJsonArray& responseList, QString& errorReason) const;
	bool handleV2ApiError(const QJsonObject& obj, QString& errorReason) const;

	void setDevicesMap( const QJsonDocument &doc );
	void setLightsMap( const QJsonDocument &doc );
	void setGroupMap( const QJsonDocument &doc );
	void setEntertainmentSrvMap( const QJsonDocument &doc );

	//Philips Hue Bridge details
	QString _deviceName;
	QString _deviceBridgeId;
	QString _deviceModel;
	int _deviceFirmwareVersion;
	QString _deviceAPIVersion;

	uint _api_major;
	uint _api_minor;
	uint _api_patch;

	bool _isPhilipsHueBridge;
	bool _isDiyHue;
	bool _isHueEntertainmentReady;
	bool _isAPIv2Ready;

	QString _applicationID;

	bool _useEntertainmentAPI;
	bool _useApiV2;

	QMap<QString,QJsonObject> _devicesMap;
	QMap<QString,QJsonObject> _lightsMap;
	QMap<QString,QJsonObject> _groupsMap;
	QMap<QString,QJsonObject> _entertainmentMap;

	int _lightsCount;
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
	int getLightsCount() const { return _lightsCount; }

	void setOnOffState(PhilipsHueLight& light, bool on, bool force = false);
	void setTransitionTime(PhilipsHueLight& light);
	void setColor(PhilipsHueLight& light, CiColor& color);
	void setColor(PhilipsHueLight &light, CiColor &color, bool force);
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
	int write(const QVector<ColorRgb>& ledValues) override;

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

	/// creates new PhilipsHueLight(s) based on user lightId with bridge feedback
	///
	/// @param map Map of lightId/value pairs of bridge
	///
	bool updateLights(const QMap<QString, QJsonObject> &map);

	///
	/// @brief Set the number of LEDs supported by the device.
	///
	/// @rparam[in] Number of device's LEDs
	//
	void setLightsCount(int lightsCount);

	bool openStream();
	bool getStreamGroupState();
	bool setStreamGroupState(bool state);
	bool startStream();
	bool stopStream();

	int writeSingleLights(const QVector<ColorRgb>& ledValues);
	int writeStreamData(const QVector<ColorRgb>& ledValues, bool flush = false);

	QJsonObject buildSetStateCommand(PhilipsHueLight& light, bool on, const CiColor& color);

	///
	bool _switchOffOnBlack;
	/// The brightness factor to multiply on color change.
	double _brightnessFactor;
	/// Transition time in multiples of 100 ms.
	/// The default of the Hue lights is 400 ms, but we may want it snappier.
	int _transitionTime;

	bool _isInitLeds;

	/// Array of the light ids.
	QStringList _lightIds;
	/// Array to save the lamps.
	std::vector<PhilipsHueLight> _lights;

	int _lightsCount;
	int _channelsCount;
	QString _groupId;
	QString _groupName;
	QString _streamOwner;

	int _blackLightsTimeout;
	double _blackLevel;
	int _onBlackTimeToPowerOff;
	int	_onBlackTimeToPowerOn;
	bool _candyGamma;

	bool _groupStreamState;
};
