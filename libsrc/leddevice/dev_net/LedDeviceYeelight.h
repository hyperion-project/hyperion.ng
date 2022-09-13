#ifndef LEDEVICEYEELIGHT_H
#define LEDEVICEYEELIGHT_H

// LedDevice includes
#include <leddevice/LedDevice.h>

// Qt includes
#include <QTcpSocket>
#include <QHostAddress>
#include <QTcpServer>
#include <QColor>

#include <chrono>

// Constants
namespace {

// List of State Information
const char API_METHOD_POWER[] = "set_power";
const char API_METHOD_POWER_ON[] = "on";
const char API_METHOD_POWER_OFF[] = "off";

const char API_METHOD_MUSIC_MODE[] = "set_music";
const int API_METHOD_MUSIC_MODE_ON = 1;
const int API_METHOD_MUSIC_MODE_OFF = 0;

const char API_METHOD_SETRGB[] = "set_rgb";
const char API_METHOD_SETSCENE[] = "set_scene";
const char API_METHOD_GETPROP[] = "get_prop";

const char API_PARAM_EFFECT_SUDDEN[] = "sudden";
const char API_PARAM_EFFECT_SMOOTH[] = "smooth";

constexpr std::chrono::milliseconds API_PARAM_DURATION{50};
constexpr std::chrono::milliseconds API_PARAM_DURATION_POWERONOFF{1000};
constexpr std::chrono::milliseconds API_PARAM_EXTRA_TIME_DARKNESS{200};

} //End of constants
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

	API_REPLY error() const { return _error;}
	void setError(const YeelightResponse::API_REPLY replyType) { _error = replyType; }

	QJsonArray getResult() const { return _resultArray; }
	void setResult(const QJsonArray &result) { _resultArray = result; }

	int getErrorCode() const { return _errorCode; }
	void setErrorCode(int errorCode) { _errorCode = errorCode; _error = API_ERROR;}

	QString getErrorReason() const { return _errorReason; }
	void setErrorReason(const QString &errorReason) { _errorReason = errorReason; }

private:

	QJsonArray _resultArray;
	API_REPLY _error = API_OK;

	int _errorCode = 0;
	QString _errorReason;
};

///
/// Implementation of one Yeelight light.
///
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

	/// @brief Constructs one Yeelight light
	///
	/// @param[in] log Logger instance
	/// @param[in] hostname or IP-address
	/// @param[in] port, default port 55443 is used when not provided
	///
	YeelightLight( Logger *log, const QString &hostname, quint16 port);

	///
	/// @brief Destructor of the Yeelight light
	///
	virtual ~YeelightLight();

	///
	/// @brief Set the Yeelight light connectivity parameters
	///
	/// @param[in] hostname or IP-address
	/// @param[in] port, default port 55443 is used when not provided
	///
	void setHostname( const QString &hostname, quint16 port);

	///
	/// @brief Set the Yeelight light name
	///
	/// @param[in] name
	///
	void setName( const QString& name )  { _name = name; }

	///
	/// @brief Get the Yeelight light name
	///
	/// @return The Yeelight light name
	///
	QString getName() const { return _name; }

	///
	/// @brief Opens the Yeelight light connectivity
	///
	/// @return True, on success (i.e. device is open)
	///
	bool open();

	///
	/// @brief Closes the Yeelight light connectivity
	///
	/// @return True, on success (i.e. device is closed)
	///
	bool close();

	///
	/// @brief Send a command to light up Yeelight light to allow identification
	///
	/// @return True, if success
	///
	bool identify();

	///
	/// @brief Execute a Yeelight-API command
	///
	/// @param[in] command The API command request in JSON
	/// @param[in] ignoreErrors Return success, even if errors occured
	/// @return 0: success, -1: error, -2: command quota exceeded
	///
	int writeCommand( const QJsonDocument &command, bool ignoreErrors = false );

	///
	/// @brief Execute a Yeelight-API command
	///
	/// @param[in] command The API command request in JSON
	/// @param[out] result The response to the command in JSON
	/// @param[in] ignoreErrors Return success, even if errors occured
	/// @return 0: success, -1: error, -2: command quota exceeded
	///
	int writeCommand( const QJsonDocument &command, QJsonArray &result, bool ignoreErrors = false );

	///
	/// @brief Stream a Yeelight-API command
	///
	/// Yeelight must be in music mode, i.e. Streaming socket is established
	///
	/// @param[in] command The API command request in JSON
	/// @return True, on success
	///
	bool streamCommand( const QJsonDocument &command );

	///
	/// @brief Set the Yeelight light streaming socket
	///
	/// @param[in] socket
	///
	void setStreamSocket( QTcpSocket* socket );

	///
	/// @brief Power on/off on the Yeelight light
	///
	/// @param[in] on True: power on, False: power off
	///
	/// @return True, if success
	///
	bool setPower( bool on );

	///
	/// @brief Power on/off on the Yeelight light
	///
	/// @param[in] on True: power on, False: power off
	/// @param[in] effect Transition effect, sudden or smooth
	/// @param[in] duration Duration of the transition, if smooth
	/// @param[in] mode Color mode after powering on
	///
	/// @return True, if success
	///
	bool setPower( bool on, API_EFFECT effect, int duration, API_MODE mode = API_RGB_MODE );

	///
	/// @brief Set the Yeelight light to the given color (using RGB mode)
	///
	/// @param[in] color as RGB value
	///
	/// @return True, if success
	///
	bool setColorRGB( const ColorRgb &color );

	///
	/// @brief Set the Yeelight light to the given color (using HSV mode)
	///
	/// @param[in] color as RGB value
	///
	/// @return True, if success
	///
	bool setColorHSV( const ColorRgb &color );

	///
	/// @brief Set the Yeelight light effect and duration while transiting between color updates
	///
	/// @param[in] effect Transition effect, sudden or smooth
	/// @param[in] duration Duration of the transition, if smooth
	///
	void setTransitionEffect ( API_EFFECT effect ,int duration = API_PARAM_DURATION.count() );

	///
	/// @brief Set the Yeelight light brightness configuration behaviour
	///
	/// @param[in] min Minimum Brightness (in %). Every value lower than minimum will be set to minimum.
	/// @param[in] max Maximum Brightness (in %). Every value greater than maximum will be set to maximum.
	/// @param[in] switchoff True, power-off light, if brightness is lower then minimum
	/// @param[in] extraTime Additional time (in ms), which added to transition duration while powering-off
	/// @param[in] factor Brightness factor to multiply on color change.
	///
	void setBrightnessConfig (int min = 1, int max = 100, bool switchoff = false,  int extraTime = 0, double factor = 1);

	///
	/// @brief Set the Yeelight light into music-mode
	///
	/// @param[in] on True: music-mode on, False: music-mode off
	/// @param[in] hostAddress of the music-mode server
	/// @param[in] port of the music-mode server
	///
	bool setMusicMode( bool on, const QHostAddress &hostAddress = {} , int port = -1 );

	///
	/// @brief Set the wait-time between two Yeelight light commands
	///
	/// The write of a command is delayed by the given wait-time, if the last write happen in the wait-time time frame.
	/// Used to avoid that the Yeelight light runs into the quota exceed error scenario.
	/// A Yeelight light can do 60 commands/min ( -> wait-time = 1000ms).
	///
	/// @param[in] waitTime in milliseconds
	///
	void setQuotaWaitTime( int waitTime ) { _waitTimeQuota = waitTime; }

	///
	/// @brief Get the Yeelight light properties
	///
	/// @return properties as JSON-object
	///
	QJsonObject getProperties();

	///
	/// @brief Get the Yeelight light properties and store them along the Yeelight light for later access
	///
	void storeState();

	///
	/// @brief Restore the Yeelight light's original state.
	///
	/// Restore the device's state as before hyperion color streaming started.
	///
	/// @return True, if success
	///
	bool restoreState();

	///
	/// @brief Check, if light was originally powered on before hyperion color streaming started..
	///
	/// @return True, if light was on at start
	///
	bool wasOriginallyOn() const { return _power == API_METHOD_POWER_ON ? true : false; }

	///
	/// @brief Check, if the Yeelight light is ready for updates
	///
	/// @return True, if ready
	///
	bool isReady() const { return !_isInError; }

	///
	/// @brief Check, if the Yeelight light is powered on
	///
	/// @return True, if powered on
	///
	bool isOn() const { return _isOn; }

	///
	/// @brief Check, if the Yeelight light is in music-mode
	///
	/// @return True, if in music mode
	///
	bool isInMusicMode( bool deviceCheck = false );

	///
	/// @brief Set the Yeelight light in error state
	///
	/// @param[in] errorMsg The error message to be logged
	///
	void setInError( const QString& errorMsg );

	///
	/// @brief Set the Yeelight light debug-level
	///
	/// @param[in] level Debug level (0: no debug output, 1-3: verbosity level)
	///
	void setDebuglevel ( int level ) { _debugLevel = level; }

private:

	YeelightResponse handleResponse(int correlationID, QByteArray const &response );

	///
	/// @brief Build Yeelight-API command
	///
	/// @param[in] method Control method to be invoked
	/// @param[in] params Parameters for control method
	/// @return Yeelight-API command in JSON format
	///
	QJsonDocument getCommand(const QString &method, const QJsonArray &params);

	///
	/// @brief Map Yeelight light properties into the Yeelight light members for direct access
	///
	/// @param[in] properties Yeelight light's properties as JSON-Object
	///
	void mapProperties(const QJsonObject &properties);

	///
	/// @brief Write a Yeelight light specific log-line for debugging purposed
	///
	/// @param[in] logLevel Debug level (0: no debug output, 1-3: verbosity level)
	/// @param[in] msg  Log message prefix (max 20 characters)
	/// @param[in] type log message text
	/// @param[in] ... variable input to log message text
	/// 	///
	void log(int logLevel,const char* msg, const char* type, ...);

	Logger* _log;
	int _debugLevel;

	/// Error status of Yeelight light
	bool _isInError;

	/// IP address/port of the Yeelight light
	QString _host;
	quint16 _port;

	/// Yeelight light communication socket
	QTcpSocket*	 _tcpSocket;
	/// Music mode server communication socket
	QTcpSocket*	 _tcpStreamSocket;

	/// ID of last command written or streamed
	int _correlationID;
	/// Timestamp of last write
	qint64	_lastWriteTime;

	/// Last color written to Yeelight light (RGB represented as QColor)
	QColor _color;
	/// Last color written to Yeelight light (RGB represented as int)
	int _lastColorRgbValue;

	/// Yeelight light behavioural parameters
	API_EFFECT _transitionEffect;
	int _transitionDuration;
	int _extraTimeDarkness;

	int _brightnessMin;
	bool _isBrightnessSwitchOffMinimum;
	int _brightnessMax;
	double _brightnessFactor;

	QString _transitionEffectParam;

	/// Wait time to avoid quota exceed scenario
	int _waitTimeQuota;

	/// Yeelight light properties
	QJsonObject _originalStateProperties;
	QString _name;
	QString _model;
	QString _power;
	QString _fw_ver;
	int _colorRgbValue;
	int _bright;
	int _ct;

	/// Yeelight light status
	bool _isOn;
	bool _isInMusicMode;
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
	~LedDeviceYeelight() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

	///
	/// @brief Get a Yeelight device's resource properties
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	///     "port"  : port, default port 55443 is used when not provided
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to query device
	/// @return A JSON structure holding the device's properties
	///
	QJsonObject getProperties(const QJsonObject& params) override;

	///
	/// @brief Send an update to the Yeelight device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "host"  : "hostname or IP",
	///     "port"  : port, default port 55443 is used when not provided
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to address device
	///
	void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
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
	int write(const std::vector<ColorRgb> & ledValues) override;

	///
	/// @brief Power-/turn on the Nanoleaf device.
	///
	/// @brief Store the device's original state.
	///
	bool powerOn() override;

	///
	/// @brief Power-/turn off the Nanoleaf device.
	///
	/// @return True if success
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

	struct yeelightAddress {
		QString host;
		int port;

		bool operator == (yeelightAddress const& a) const
		{
			return ((host == a.host) && (port == a.port));
		}
	};

	enum COLOR_MODEL{
		MODEL_HSV,
		MODEL_RGB
	};

	///
	/// @brief Start music-mode server
	///
	/// @return True, if music mode server is running
	///
	bool startMusicModeServer();

	///
	/// @brief Stop music-mode server
	///
	/// @return True, if music mode server has been stopped
	///
	bool stopMusicModeServer();

	///
	/// @brief Update list of Yeelight lights handled by the LED-device
	///
	/// @param[in] list List of Yeelight lights
	///
	/// @return False, if no lights were provided
	///
	bool updateLights(const QVector<yeelightAddress> &list);

	///
	/// @brief Set the number of Yeelight lights handled by the LED-device
	///
	/// @param[in] lightsCount Number of Yeelight lights
	///
	void setLightsCount( unsigned int lightsCount )	{ _lightsCount = lightsCount; }

	///
	/// @brief Get the number of Yeelight lights handled by the LED-device
	///
	/// @return Number of Yeelight lights
	///
	uint getLightsCount() const { return _lightsCount; }

	///
	/// @brief Discover Yeelight devices available (for configuration).
	/// Yeelight specific UDP Broadcast discovery
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonArray discover();

	/// Array of the Yeelight addresses handled by the LED-device
	QVector<yeelightAddress> _lightsAddressList;

	/// Array to save the lights
	std::vector<YeelightLight> _lights;
	unsigned int _lightsCount;

	/// Yeelight configuration/behavioural parameters
	int _outputColorModel;
	YeelightLight::API_EFFECT _transitionEffect;
	int _transitionDuration;
	int _extraTimeDarkness;

	int _brightnessMin;
	bool _isBrightnessSwitchOffMinimum;
	int _brightnessMax;
	double _brightnessFactor;

	int _waitTimeQuota;

	int _debuglevel;

	///Music mode Server details
	QHostAddress _musicModeServerAddress;
	int _musicModeServerPort;
	QTcpServer* _tcpMusicModeServer = nullptr;

};

#endif // LEDEVICEYEELIGHT_H
