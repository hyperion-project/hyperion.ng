#ifndef LEDEVICEYEELIGHT_H
#define LEDEVICEYEELIGHT_H

// Leddevice includes
#include <leddevice/LedDevice.h>

// Qt includes
#include <QTcpSocket>
#include <QHostAddress>
#include <QTcpServer>
#include <QColor>

// List of State Information
static const char API_METHOD_POWER[] = "set_power";
static const char API_METHOD_POWER_ON[] = "on";
static const char API_METHOD_POWER_OFF[] = "off";

static const char API_METHOD_MUSIC_MODE[] = "set_music";
static const int API_METHOD_MUSIC_MODE_ON = 1;
static const int API_METHOD_MUSIC_MODE_OFF = 0;

static const char API_METHOD_SETRGB[] = "set_rgb";
static const char API_METHOD_SETSCENE[] = "set_scene";
static const char API_METHOD_GETPROP[] = "get_prop";

static const char API_PARAM_EFFECT_SUDDEN[] = "sudden";
static const char API_PARAM_EFFECT_SMOOTH[] = "smooth";

static const int  API_PARAM_DURATION = 50;
static const int  API_PARAM_DURATION_POWERONOFF = 1000;
static const int  API_PARAM_EXTRA_TIME_DARKNESS = 200;

///
/// Response object for Yeelight-API calls and JSON-responses
///
class YeelightResponse
{
public:

	enum API_REPLY{
		API_OK,
		API_ERROR,
		API_NOTIFICATION,
		};

	explicit YeelightResponse() {}

	API_REPLY error() { return _error;}
	void setError(const YeelightResponse::API_REPLY replyType) { _error = replyType; }

	QJsonArray getResult() const { return _resultArray; }
	void setResult(const QJsonArray &result) { _resultArray = result; }

	int getErrorCode() const { return _errorCode; }
	void setErrorCode(const int &errorCode) { _errorCode = errorCode; _error = API_ERROR;}

	QString getErrorReason() const { return _errorReason; }
	void setErrorReason(const QString &errorReason) { _errorReason = errorReason; }

private:

	QJsonArray _resultArray;
	API_REPLY _error = API_OK;

	int _errorCode = 0;
	QString _errorReason;
};

/**
 * Simple class to hold the id, the latest color, the color space and the original state.
 */
class YeelightLight
{

public:

	enum API_EFFECT{
		API_EFFECT_SMOOTH,
		API_EFFECT_SUDDEN
	};

	enum API_MODE{
		API_TURN_ON_MODE,
		API_CT_MODE,
		API_RGB_MODE,
		API_HSV_MODE,
		API_COLOR_FLOW_MODE,
		API_NIGHT_LIGHT_MODE
	};

	///
	/// Constructs the light.
	///
	/// @param log the logger
	/// @param id the light id
	///
	YeelightLight( Logger *log, const QString &hostname, int port);
	~YeelightLight();

	bool open();

	bool close();

	int writeCommand( const QJsonDocument &command );
	int writeCommand( const QJsonDocument &command, QJsonArray &result );

	bool streamCommand( const QJsonDocument &command );


	void setHostname( const QString& hostname, int port);

	void setStreamSocket( QTcpSocket* socket );

	///
	/// @param on
	///
	bool setPower( bool on );

	bool setPower( bool on, API_EFFECT effect, int duration, API_MODE mode = API_RGB_MODE );

	bool setColorRGB( ColorRgb color );
	bool setColorRGB2( ColorRgb color );
	bool setColorHSV( ColorRgb color );

	void setTransitionEffect ( API_EFFECT effect ,int duration = API_PARAM_DURATION );

	void setBrightnessConfig (int min = 1, int max = 100, bool switchoff = false,  int extraTime = 0, double factor = 1);

	void setQuotaWaitTime( const int waitTime ) { _waitTimeQuota = waitTime; }
	bool setMusicMode( bool on, QHostAddress ipAddress = {} , int port = -1 );

	QJsonObject getProperties();
	void storeProperties();

	bool identify();

	void setName( const QString& name )  { _name = name; }
	QString getName()const { return _name; }

	bool isReady() const { return !_isInError; }
	bool isOn() const { return _isOn; }
	bool isInMusicMode( bool deviceCheck = false );

	/// Set device in error state
	///
	/// @param errorMsg The error message to be logged
	///
	void setInError( const QString& errorMsg );

	void setDebuglevel ( int level ) { _debugLevel = level; }

private:

	YeelightResponse handleResponse(int correlationID, QByteArray const &response );

	void saveOriginalState(const QJsonObject& values);

	//QString getCommand(const QString &method, const QString &params);
	QJsonDocument getCommand(const QString &method, const QJsonArray &params);

	void mapProperties(const QMap<QString, QString> propertyList);
	void mapProperties(const QJsonObject &properties);

	void log(const int logLevel,const char* msg, const char* type, ...);

	Logger* _log;
	int _debugLevel;

	bool _isInError;

	/// Ip address of the Yeelight
	QString _host;
	int _port;
	QString _defaultHost;

	QTcpSocket*	 _tcpSocket;
	int _correlationID;

	QTcpSocket*	 _tcpStreamSocket;

	QColor _color;
	int _colorRgbValue;
	int _bright;
	int _ct;

	API_EFFECT _transitionEffect;
	int _transitionDuration;
	int _extraTimeDarkness;

	int _brightnessMin;
	bool _isBrightnessSwitchOffMinimum;
	int _brightnessMax;
	double _brightnessFactor;

	QString _transitionEffectParam;

	int _waitTimeQuota;

	// Light properties
	QJsonObject _properties;
	QString _name;
	QString _model;
	QString _power;
	QString _fw_ver;

	bool _isOn;
	bool _isInMusicMode;

	/// timestamp of last write
	qint64	_lastWriteTime;
};

///
/// Implementation of the LedDevice interface for sending to
/// Yeelight devices via network
///
class LedDeviceYeelight : public LedDevice
{
public:

	///
	/// @brief Constructs a Yeelight LED-device serving multiple lights
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceYeelight(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	virtual ~LedDeviceYeelight() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Discover Yeelight devices available (for configuration).
	///
	/// @return A JSON structure holding a list of devices found
	///
	virtual QJsonObject discover() override;

	///
	/// @brief Get a Yeelight device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "hostname"  : "hostname or IP",
	///     "port"      : port, default port 55443 is used when not provided
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	virtual QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the Yeelight device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "hostname"  : "hostname or IP",
	///     "port"      : port, default port 55443 is used when not provided
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to address device
	///
	virtual void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	virtual bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	virtual int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	virtual int close() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues) override;

	///
	/// @brief Power-/turn on the Nanoleaf device.
	///
	/// @brief Store the device's original state.
	///
	virtual bool powerOn() override;

	///
	/// @brief Power-/turn off the Nanoleaf device.
	///
	/// @return True if success
	///
	virtual bool powerOff() override;

	///
	/// @brief Store the device's original state.
	///
	/// Save the device's state before hyperion color streaming starts allowing to restore state during switchOff().
	///
	/// @return True if success
	///
	virtual bool storeState() override;

private:

	struct yeelightAddress {
		QString host;
		int port;

		bool operator == (yeelightAddress const& a) const
		{
			return ((host == a.host) && (port == a.port));
		}
	};

	bool openMusicModeServer();
	bool closeMusicModeServer();

	bool updateLights(QVector<yeelightAddress>& list);

	void setLightsCount( unsigned int lightsCount )	{ _lightsCount = lightsCount; }
	uint getLightsCount() { return _lightsCount; }

	///
	/// Get Yeelight command
	///
	/// @param method
	/// @param parameters
	/// @return command to execute
	///
	QString getCommand(const QString &method, const QString &params);

	/// Array of the Yeelight addresses.
	QVector<yeelightAddress> _lightsAddressList;

	/// Array to save the lamps.
	std::vector<YeelightLight> _lights;
	unsigned int _lightsCount;

	int _outputColorModel;
	YeelightLight::API_EFFECT _transitionEffect;
	int _transitionDuration;
	int _extraTimeDarkness;

	int _brightnessMin;
	bool _isBrightnessSwitchOffMinimum;
	int _brightnessMax;
	/// The brightness factor to multiply on color change.
	double _brightnessFactor;

	int _waitTimeQuota;

	int _debuglevel;

	QHostAddress _musicModeServerAddress;
	quint16 _musicModeServerPort;
	QTcpServer* _tcpMusicModeServer = nullptr;

};

#endif // LEDEVICEYEELIGHT_H
