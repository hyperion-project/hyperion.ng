#pragma once

// STL includes
#include <set>
#include <string>
#include <stdarg.h>

// Qt includes
#include <QNetworkAccessManager>

// Leddevice includes
#include <leddevice/LedDevice.h>
#include "ProviderUdpSSL.h"

// Forward declaration
struct CiColorTriangle;

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
	static CiColor rgbToCiColor(double red, double green, double blue, CiColorTriangle colorSpace);

	///
	/// @param p the color point to check
	///
	/// @return true if the color point is covered by the lamp color space
	///
	static bool isPointInLampsReach(CiColor p, CiColorTriangle colorSpace);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the cross product between p1 and p2
	///
	static double crossProduct(CiColor p1, CiColor p2);

	///
	/// @param a reference point one
	///
	/// @param b reference point two
	///
	/// @param p the point to which the closest point is to be found
	///
	/// @return the closest color point of p to a and b
	///
	static CiColor getClosestPointToPoint(CiColor a, CiColor b, CiColor p);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the distance between the two points
	///
	static double getDistanceBetweenTwoPoints(CiColor p1, CiColor p2);
};

bool operator==(const CiColor& p1, const CiColor& p2);
bool operator!=(const CiColor& p1, const CiColor& p2);

/**
 * Color triangle to define an available color space for the hue lamps.
 */
struct CiColorTriangle
{
	CiColor red, green, blue;
};

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
	void setTransitionTime(unsigned int _transitionTime);

	///
	/// @param color the color to set
	///
	void setColor(const CiColor& _color);


	unsigned int getId() const;

	bool getOnOffState() const;
	unsigned int getTransitionTime() const;
	CiColor getColor() const;

	///
	/// @return the color space of the light determined by the model id reported by the bridge.
	CiColorTriangle getColorSpace() const;


	QString getOriginalState();


private:

	void saveOriginalState(const QJsonObject& values);

	Logger* _log;
	/// light id
	unsigned int _id;
	unsigned int _ledidx;
	bool _on;
	unsigned int _transitionTime;
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
	~LedDevicePhilipsHueBridge();

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig) override;

	///
	/// @param route the route of the POST request.
	///
	/// @param content the content of the POST request.
	///
	QJsonDocument post(const QString& route, const QString& content);

	void setLightState(unsigned int lightId = 0, QString state = "");

	const QMap<quint16,QJsonObject>& getLightMap();

	const QMap<quint16,QJsonObject>& getGroupMap();

	QString getGroupName(unsigned int groupId = 0);

	QJsonArray getGroupLights(unsigned int groupId = 0);

//	/// Set device in error state
//	///
//	/// @param errorMsg The error message to be logged
//	///
//	virtual void setInError( const QString& errorMsg) override;

public slots:
	///
	/// Connect to bridge to check availbility and user
	///
	virtual int open(void) override;
	virtual int open( const QString& hostname, const QString& port, const QString& username );

	//signals:
	//	///
	//	///	Emits with a QMap of current bridge light/value pairs
	//	///
	//	void newLights(QMap<quint16,QJsonObject> map);
protected:

	/// Ip address of the bridge
	QString _hostname;
	QString _api_port;
	/// User name for the API ("newdeveloper")
	QString _username;

	bool _useHueEntertainmentAPI;
	bool _logCommands;

	QJsonDocument getGroupState( unsigned int groupId );
	QJsonDocument setGroupState( unsigned int groupId, bool state);

	bool isStreamOwner(QString streamOwner);
	bool initMaps();

	void log(const char* msg, const char* type, ...);

private:

	///
	/// Discover device via SSDP identifiers
	///
	/// @return True, if device was found
	///
	bool discoverDevice();

	///
	/// Get command as url
	///
	/// @param host Hostname or IP
	/// @param port IP-Port
	/// @param _auth_token Authorization token
	/// @param Endpoint command for request
	/// @return Url to execute endpoint/command
	///
	QString getUrl(QString host, QString port, QString auth_token, QString endpoint) const;

	///
	/// Execute GET request
	///
	/// @param url GET request for url
	/// @return Response from device
	///
	QJsonDocument getJson(QString url);

	///
	/// Execute PUT request
	///
	/// @param Url for PUT request
	/// @param json Command for request
	/// @return Response from device
	///
	QJsonDocument putJson(QString url, QString json);

	///
	/// Handle replys for GET and PUT requests
	///
	/// @param reply Network reply
	/// @return Response for request, if no error
	///
	QJsonDocument handleReply(QNetworkReply* const &reply );

	QJsonDocument getAllBridgeInfos();
	void setBridgeConfig( QJsonDocument doc );
	void setLightsMap( QJsonDocument doc );
	void setGroupMap( QJsonDocument doc );

	/// QNetworkAccessManager  for sending requests.
	QNetworkAccessManager* _networkmanager;

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
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	explicit LedDevicePhilipsHue(const QJsonObject &deviceConfig);

	///
	/// Destructor of this device
	///
	virtual ~LedDevicePhilipsHue();

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig) override;

	/// Switch the device on
	virtual int switchOn() override;

	/// Switch the device off
	virtual int switchOff() override;

	/// creates new PhilipsHueLight(s) based on user lightid with bridge feedback
	///
	/// @param map Map of lightid/value pairs of bridge
	///
	void newLights(QMap<quint16, QJsonObject> map);

	unsigned int getLightsCount() const { return _lightsCount; }
	void setLightsCount( unsigned int lightsCount);

	bool initStream();
	bool getStreamGroupState();
	bool setStreamGroupState(bool state);
	bool startStream();
	bool stopStream();

	void setOnOffState(PhilipsHueLight& light, bool on);
	void setTransitionTime(PhilipsHueLight& light);
	void setColor(PhilipsHueLight& light, CiColor& color);
	void setState(PhilipsHueLight& light, bool on, const CiColor& color);

	void restoreOriginalState();

public slots:

	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	virtual void close() override;

private slots:
	/// creates new PhilipsHueLight(s) based on user lightid with bridge feedback
	///
	/// @param map Map of lightid/value pairs of bridge
	///
	bool updateLights(QMap<quint16, QJsonObject> map);

	void noSignalTimeout();

protected:

	///
	/// Opens and initiatialises the output device
	///
	/// @return Zero on succes (i.e. device is ready and enabled) else negative
	///
	virtual int open() override;

	///
	/// Get Philips Hue device details and configuration
	///
	/// @return True, if Nanoleaf device capabilities fit configuration
	///
	bool initLeds();
	bool reinitLeds();

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

private:

	bool setLights();

	int writeSingleLights(const std::vector<ColorRgb>& ledValues);

	void writeStream();

	bool noSignalDetection();

	void stopTimeoutTimer();

	QByteArray prepareStreamData();

	///
	bool _switchOffOnBlack;
	/// The brightness factor to multiply on color change.
	double _brightnessFactor;
		/// Transition time in multiples of 100 ms.
	/// The default of the Hue lights is 400 ms, but we may want it snapier.
	unsigned int _transitionTime;

	bool _isRestoreOrigState;
	bool _lightStatesRestored;
	bool _isInitLeds;

	/// Array of the light ids.
	std::vector<unsigned int> _lightIds;
	/// Array to save the lamps.
	std::vector<PhilipsHueLight> _lights;

	unsigned int _lightsCount;

	double _brightnessMin;
	double _brightnessMax;

	bool _allLightsBlack;

	QTimer* _blackLightsTimer;
	unsigned int _blackLightsTimeout;
	double _brightnessThreshold;

	bool _stopConnection;

	unsigned int _groupId;

	QString _groupName;
	QString _streamOwner;

	int start_retry_left;
	int stop_retry_left;
};
