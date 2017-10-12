#pragma once

// stl includes
#include <list>
#include <QMap>

// QT includes
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QSize>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QTimer>
#include <QFileSystemWatcher>

// hyperion-utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/VideoMode.h>
#include <utils/GrabbingMode.h>

// Hyperion includes
#include <hyperion/LedString.h>
#include <hyperion/PriorityMuxer.h>
#include <hyperion/ColorAdjustment.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ComponentRegister.h>

// Effect engine includes
#include <effectengine/EffectDefinition.h>
#include <effectengine/ActiveEffectDefinition.h>
#include <effectengine/EffectSchema.h>

// KodiVideoChecker includes
#include <kodivideochecker/KODIVideoChecker.h>
#include <bonjour/bonjourservicebrowser.h>
#include <bonjour/bonjourserviceresolver.h>

// Forward class declaration
class LedDevice;
class LinearColorSmoothing;
class RgbTransform;
class EffectEngine;
class RgbChannelAdjustment;
class MultiColorAdjustment;
class KODIVideoChecker;

///
/// The main class of Hyperion. This gives other 'users' access to the attached LedDevice through
/// the priority muxer.
///
class Hyperion : public QObject
{
	Q_OBJECT
public:
	///  Type definition of the info structure used by the priority muxer
	typedef PriorityMuxer::InputInfo InputInfo;
	typedef QMap<QString,int> PriorityRegister;
	typedef QMap<QString,BonjourRecord>  BonjourRegister;
	///
	/// RGB-Color channel enumeration
	///
	enum RgbChannel
	{
		BLACK, WHITE, RED, GREEN, BLUE, CYAN, MAGENTA, YELLOW, INVALID
	};

	///
	/// Destructor; cleans up resources
	///
	~Hyperion();

	///
	/// free all alocated objects, should be called only from constructor or before restarting hyperion
	///
	void freeObjects(bool emitCloseSignal=false);

	///
	/// @brief creates a new Hyperion instance, usually called from the Hyperion Daemon
	/// @param[in] qjsonConfig   The configuration file
	/// @param[in] rootPath      Root path of all hyperion userdata
	/// @return                  Hyperion instance pointer
	///
	static Hyperion* initInstance(const QJsonObject& qjsonConfig, const QString configFile, const QString rootPath);

	///
	/// @brief Get a pointer of this Hyperion instance
	/// @return                  Hyperion instance pointer
	///
	static Hyperion* getInstance();

	///
	/// Returns the number of attached leds
	///
	unsigned getLedCount() const;

	QSize getLedGridSize() const { return _ledGridSize; };

	///
	/// Returns the current priority
	///
	/// @return The current priority
	///
	int getCurrentPriority() const;

	///
	/// Returns true if current priority is given priority
	///
	/// @return bool
	///
	bool isCurrentPriority(const int priority) const;

	///
	/// Returns a list of active priorities
	///
	/// @return The list with priorities
	///
	QList<int> getActivePriorities() const;

	///
	/// Returns the information of a specific priorrity channel
	///
	/// @param[in] priority  The priority channel
	///
	/// @return The information of the given
	///
	/// @throw std::runtime_error when the priority channel does not exist
	///
	const InputInfo& getPriorityInfo(const int priority) const;

	/// Reload the list of available effects
	void reloadEffects();

	/// Get the list of available effects
	/// @return The list of available effects
	const std::list<EffectDefinition> &getEffects() const;

	/// Get the list of active effects
	/// @return The list of active effects
	const std::list<ActiveEffectDefinition> &getActiveEffects();

	/// Get the list of available effect schema files
	/// @return The list of available effect schema files
	const std::list<EffectSchema> &getEffectSchemas();

	/// gets the current json config object
	/// @return json config
	const QJsonObject& getQJsonConfig() { return _qjsonConfig; };

	/// get filename of configfile
	/// @return the current config filename
	QString getConfigFileName() { return _configFile; };

	/// register a input source to a priority channel
	/// @param name uniq name of input source
	/// @param origin External setter
	/// @param priority priority channel
	void registerPriority(const QString &name, const int priority);

	/// unregister a input source to a priority channel
	/// @param name uniq name of input source
	void unRegisterPriority(const QString &name);

	/// gets current priority register
	/// @return the priority register
	const PriorityRegister& getPriorityRegister() { return _priorityRegister; }

	/// enable/disable automatic/priorized source selection
	/// @param enabled the state
	void setSourceAutoSelectEnabled(bool enabled);

	/// set current input source to visible
	/// @param priority the priority channel which should be vidible
	/// @return true if success, false on error
	bool setCurrentSourcePriority(int priority );

	/// gets current state of automatic/priorized source selection
	/// @return the state
	bool sourceAutoSelectEnabled() { return _sourceAutoSelectEnabled; };

	///
	/// Enable/Disable components during runtime
	///
	/// @param component The component [SMOOTHING, BLACKBORDER, KODICHECKER, FORWARDER, UDPLISTENER, BOBLIGHT_SERVER, GRABBER]
	/// @param state The state of the component [true | false]
	///
	void setComponentState(const hyperion::Components component, const bool state);

	ComponentRegister& getComponentRegister() { return _componentRegister; };

	bool configModified() { return _configMod; };

	bool configWriteable() { return _configWrite; };

	/// gets the methode how image is maped to leds
	int getLedMappingType() { return _ledMAppingType; };

	/// get the configuration
	QJsonObject getConfig() { return _qjsonConfig; };

	/// get the root path for all hyperion user data files
	QString getRootPath() { return _rootPath; };

	/// unique id per instance
	QString id;

	int getLatchTime() const;

	/// forward smoothing config
	unsigned addSmoothingConfig(int settlingTime_ms, double ledUpdateFrequency_hz=25.0, unsigned updateDelay=0);

	VideoMode getCurrentVideoMode() { return _videoMode; };
	GrabbingMode getCurrentGrabbingMode() { return _grabbingMode; };

public slots:
	///
	/// Writes a single color to all the leds for the given time and priority
	///
	/// @param[in] priority The priority of the written color
	/// @param[in] ledColor The color to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given color [ms]
	///
	void setColor(int priority, const ColorRgb &ledColor, const int timeout_ms, bool clearEffects = true);

	///
	/// Writes the given colors to all leds for the given time and priority
	///
	/// @param[in] priority The priority of the written colors
	/// @param[in] ledColors The colors to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given colors [ms]
	/// @param[in] component The current component
	/// @param[in] origin Who set it
	/// @param[in] smoothCfg smoothing config id
	///
	void setColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms, bool clearEffects = true, hyperion::Components component=hyperion::COMP_INVALID, const QString origin="System", unsigned smoothCfg=SMOOTHING_MODE_DEFAULT);

	///
	/// Writes the given colors to all leds for the given time and priority
	///
	/// @param[in] priority The priority of the written colors
	/// @param[in] ledColors The colors to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given colors [ms]
	///
	void setImage(int priority, const Image<ColorRgb> & image, int duration_ms);

	///
	/// Returns the list with unique adjustment identifiers
	/// @return The list with adjustment identifiers
	///
	const QStringList & getAdjustmentIds() const;

	///
	/// Returns the ColorAdjustment with the given identifier
	/// @return The adjustment with the given identifier (or nullptr if the identifier does not exist)
	///
	ColorAdjustment * getAdjustment(const QString& id);

	///
	/// Returns  MessageForwarder Object
	/// @return instance of message forwarder object
	///
	MessageForwarder * getForwarder();

	/// Tell Hyperion that the corrections have changed and the leds need to be updated
	void adjustmentsUpdated();

	///
	/// Clears the given priority channel. This will switch the led-colors to the colors of the next
	/// lower priority channel (or off if no more channels are set)
	///
	/// @param[in] priority  The priority channel
	///
	void clear(int priority);

	///
	/// Clears all priority channels. This will switch the leds off until a new priority is written.
	///
	void clearall(bool forceClearAll=false);

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	/// @param effectName Name of the effec to run
	///	@param priority The priority channel of the effect
	/// @param timeout The timeout of the effect (after the timout, the effect will be cleared)
	int setEffect(const QString & effectName, int priority, int timeout = -1, const QString & origin="System");

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	/// @param effectName Name of the effec to run
	/// @param args arguments of the effect script
	///	@param priority The priority channel of the effect
	/// @param timeout The timeout of the effect (after the timout, the effect will be cleared)
	int setEffect(const QString & effectName, const QJsonObject & args, int priority,
				  int timeout = -1, const QString & pythonScript = "", const QString & origin="System");

	/// sets the methode how image is maped to leds
	void setLedMappingType(int mappingType);

	///
	Hyperion::BonjourRegister getHyperionSessions();

	/// Slot which is called, when state of hyperion has been changed
	void hyperionStateChanged();

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(VideoMode mode);

	///
	/// Set the grabbing mode
	/// @param[in] mode The new grabbing mode
	///
	void setGrabbingMode(const GrabbingMode mode);


public:
	static Hyperion *_hyperion;

	static ColorOrder createColorOrder(const QJsonObject & deviceConfig);
	/**
	 * Construct the 'led-string' with the integration area definition per led and the color
	 * ordering of the RGB channels
	 * @param ledsConfig   The configuration of the led areas
	 * @param deviceOrder  The default RGB channel ordering
	 * @return The constructed ledstring
	 */
	static LedString createLedString(const QJsonValue & ledsConfig, const ColorOrder deviceOrder);
	static LedString createLedStringClone(const QJsonValue & ledsConfig, const ColorOrder deviceOrder);

	static MultiColorAdjustment * createLedColorsAdjustment(const unsigned ledCnt, const QJsonObject & colorAdjustmentConfig);
	static ColorAdjustment * createColorAdjustment(const QJsonObject & adjustmentConfig);
	static RgbTransform * createRgbTransform(const QJsonObject& colorConfig);
	static RgbChannelAdjustment * createRgbChannelAdjustment(const QJsonObject & colorConfig, const QString channelName, const int defaultR, const int defaultG, const int defaultB);

	static LinearColorSmoothing * createColorSmoothing(const QJsonObject & smoothingConfig, LedDevice* leddevice);
	static MessageForwarder * createMessageForwarder(const QJsonObject & forwarderConfig);
	static QSize getLedLayoutGridSize(const QJsonValue& ledsConfig);

signals:
	/// Signal which is emitted when a priority channel is actively cleared
	/// This signal will not be emitted when a priority channel time out
	void channelCleared(int priority);

	/// Signal which is emitted when all priority channels are actively cleared
	/// This signal will not be emitted when a priority channel time out
	void allChannelsCleared();

	void componentStateChanged(const hyperion::Components component, bool enabled);

	void imageToLedsMappingChanged(int mappingType);
	void emitImage(int priority, const Image<ColorRgb> & image, const int timeout_ms);
	void closing();

	/// Signal which is emitted, when a new json message should be forwarded
	void forwardJsonMessage(QJsonObject);

	/// Signal which is emitted, after the hyperionStateChanged has been processed with a emit count blocker (250ms interval)
	void sendServerInfo();

	/// Signal emitted when a 3D movie is detected
	void videoMode(VideoMode mode);

	void grabbingMode(GrabbingMode mode);

private slots:
	///
	/// Updates the priority muxer with the current time and (re)writes the led color with applied
	/// transforms.
	///
	void update();

	void currentBonjourRecordsChanged(const QList<BonjourRecord> &list);
	void bonjourRecordResolved(const QHostInfo &hostInfo, int port);
	void bonjourResolve();

	/// check for configWriteable and modified changes, called by _fsWatcher or fallback _cTimer
	void checkConfigState(QString cfile = NULL);

private:

	///
	/// Constructs the Hyperion instance based on the given Json configuration
	///
	/// @param[in] qjsonConfig The Json configuration
	///
	Hyperion(const QJsonObject& qjsonConfig, const QString configFile, const QString rootPath);

	/// The specifiation of the led frame construction and picture integration
	LedString _ledString;

	/// specifiation of cloned leds
	LedString _ledStringClone;

	std::vector<ColorOrder> _ledStringColorOrder;

	/// The priority muxer
	PriorityMuxer _muxer;

	/// The adjustment from raw colors to led colors
	MultiColorAdjustment * _raw2ledAdjustment;

	/// The actual LedDevice
	LedDevice * _device;

	/// The smoothing LedDevice
	LinearColorSmoothing * _deviceSmooth;

	/// Effect engine
	EffectEngine * _effectEngine;

	// proto and json Message forwarder
	MessageForwarder * _messageForwarder;

	// json configuration
	const QJsonObject& _qjsonConfig;

	/// the name of config file
	QString _configFile;

	/// root path for all hyperion user data files
	QString _rootPath;

	/// The timer for handling priority channel timeouts
	QTimer _timer;
	QTimer _timerBonjourResolver;

	/// buffer for leds
	std::vector<ColorRgb> _ledBuffer;

	/// Logger instance
	Logger * _log;

	/// count of hardware leds
	unsigned _hwLedCount;

	ComponentRegister _componentRegister;

	/// register of input sources and it's prio channel
	PriorityRegister _priorityRegister;

	/// flag indicates state for autoselection of input source
	bool _sourceAutoSelectEnabled;

	/// holds the current priority channel that is manualy selected
	int _currentSourcePriority;

	QByteArray _configHash;

	QSize _ledGridSize;

	int _ledMAppingType;

	hyperion::Components   _prevCompId;
	BonjourServiceBrowser  _bonjourBrowser;
	BonjourServiceResolver _bonjourResolver;
	BonjourRegister        _hyperionSessions;
	QString                _bonjourCurrentServiceToResolve;

	/// Observe filesystem changes (_configFile), if failed use Timer
	QFileSystemWatcher _fsWatcher;
	QTimer _cTimer;

	/// holds the prev states of configWriteable and modified
	bool _prevConfigMod = false;
	bool _prevConfigWrite = true;

	/// holds the current states of configWriteable and modified
	bool _configMod = false;
	bool _configWrite = true;

	/// timers to handle severinfo blocking
	QTimer _fsi_timer;
	QTimer _fsi_blockTimer;

	VideoMode _videoMode;
	GrabbingMode _grabbingMode;
};
