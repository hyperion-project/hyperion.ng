#pragma once

// stl includes
#include <list>
#include <map>

// QT includes
#include <QObject>
#include <QString>
#include <QTimer>
#include <QSize>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

// hyperion-utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/Logger.h>
#include <utils/Components.h>

// Hyperion includes
#include <hyperion/LedString.h>
#include <hyperion/PriorityMuxer.h>
#include <hyperion/ColorTransform.h>
#include <hyperion/ColorAdjustment.h>
#include <hyperion/MessageForwarder.h>
#include <hyperion/ComponentRegister.h>

// Effect engine includes
#include <effectengine/EffectDefinition.h>
#include <effectengine/ActiveEffectDefinition.h>
#include <effectengine/EffectSchema.h>

// KodiVideoChecker includes
#include <kodivideochecker/KODIVideoChecker.h>

// Forward class declaration
class LedDevice;
class LinearColorSmoothing;
class ColorTransform;
class EffectEngine;
class HsvTransform;
class HslTransform;
class RgbChannelTransform;
class RgbChannelAdjustment;
class MultiColorTransform;
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
	typedef std::map<std::string,int> PriorityRegister;

	///
	/// RGB-Color channel enumeration
	///
	enum RgbChannel
	{
		RED, GREEN, BLUE, INVALID
	};

	///
	/// Enumeration of the possible color (color-channel) transforms
	///
	enum Transform
	{
		SATURATION_GAIN, VALUE_GAIN, THRESHOLD, GAMMA, BLACKLEVEL, WHITELEVEL
	};

	///
	/// Destructor; cleans up resources
	///
	~Hyperion();

	///
	/// free all alocated objects, should be called only from constructor or before restarting hyperion
	///
	void freeObjects();

	static Hyperion* initInstance(const QJsonObject& qjsonConfig, const QString configFile);
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
	std::string getConfigFileName() { return _configFile.toStdString(); };

	/// register a input source to a priority channel
	/// @param name uniq name of input source
	/// @param priority priority channel
	void registerPriority(const std::string name, const int priority);
	
	/// unregister a input source to a priority channel
	/// @param name uniq name of input source
	void unRegisterPriority(const std::string name);
	
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

	bool configModified();

	bool configWriteable();

	/// gets the methode how image is maped to leds
	int getLedMappingType() { return _ledMAppingType; };
	
	int getConfigVersionId() { return _configVersionId; };

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
	///
	void setColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms, bool clearEffects = true, hyperion::Components component=hyperion::COMP_INVALID);

	///
	/// Writes the given colors to all leds for the given time and priority
	///
	/// @param[in] priority The priority of the written colors
	/// @param[in] ledColors The colors to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given colors [ms]
	///
	void setImage(int priority, const Image<ColorRgb> & image, int duration_ms);

	///
	/// Returns the list with unique transform identifiers
	/// @return The list with transform identifiers
	///
	const std::vector<std::string> & getTransformIds() const;

	///
	/// Returns the list with unique adjustment identifiers
	/// @return The list with adjustment identifiers
	///
	const std::vector<std::string> & getAdjustmentIds() const;
	
	///
	/// Returns the ColorTransform with the given identifier
	/// @return The transform with the given identifier (or nullptr if the identifier does not exist)
	///
	ColorTransform * getTransform(const std::string& id);

	///
	/// Returns the ColorAdjustment with the given identifier
	/// @return The adjustment with the given identifier (or nullptr if the identifier does not exist)
	///
	ColorAdjustment * getAdjustment(const std::string& id);

	///
	/// Returns  MessageForwarder Object
	/// @return instance of message forwarder object
	///
	MessageForwarder * getForwarder();

	/// Tell Hyperion that the transforms have changed and the leds need to be updated
	void transformsUpdated();

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
	void clearall();

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	/// @param effectName Name of the effec to run
	///	@param priority The priority channel of the effect
	/// @param timeout The timeout of the effect (after the timout, the effect will be cleared)
	int setEffect(const QString & effectName, int priority, int timeout = -1);

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	/// @param effectName Name of the effec to run
	/// @param args arguments of the effect script
	///	@param priority The priority channel of the effect
	/// @param timeout The timeout of the effect (after the timout, the effect will be cleared)
	int setEffect(const QString & effectName, const QJsonObject & args, int priority, int timeout = -1, QString pythonScript = "");

	/// sets the methode how image is maped to leds
	void setLedMappingType(int mappingType);

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

	static MultiColorTransform * createLedColorsTransform(const unsigned ledCnt, const QJsonObject & colorTransformConfig);
	static MultiColorAdjustment * createLedColorsAdjustment(const unsigned ledCnt, const QJsonObject & colorAdjustmentConfig);
	static ColorTransform * createColorTransform(const QJsonObject & transformConfig);
	static ColorAdjustment * createColorAdjustment(const QJsonObject & adjustmentConfig);
	static HsvTransform * createHsvTransform(const QJsonObject & hsvConfig);
	static HslTransform * createHslTransform(const QJsonObject & hslConfig);
	static RgbChannelTransform * createRgbChannelTransform(const QJsonObject& colorConfig);
	static RgbChannelAdjustment * createRgbChannelCorrection(const QJsonObject& colorConfig);
	static RgbChannelAdjustment * createRgbChannelAdjustment(const QJsonObject& colorConfig, const RgbChannel color);

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

private slots:
	///
	/// Updates the priority muxer with the current time and (re)writes the led color with applied
	/// transforms.
	///
	void update();

private:
	
	///
	/// Constructs the Hyperion instance based on the given Json configuration
	///
	/// @param[in] qjsonConfig The Json configuration
	///
	Hyperion(const QJsonObject& qjsonConfig, const QString configFile);

	/// The specifiation of the led frame construction and picture integration
	LedString _ledString;

	/// specifiation of cloned leds
	LedString _ledStringClone;

	std::vector<ColorOrder> _ledStringColorOrder;
	/// The priority muxer
	PriorityMuxer _muxer;

	/// The transformation from raw colors to led colors
	MultiColorTransform * _raw2ledTransform;

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

	// the name of config file
	QString _configFile;

	/// The timer for handling priority channel timeouts
	QTimer _timer;

	/// buffer for leds
	std::vector<ColorRgb> _ledBuffer;

	/// Logger instance
	Logger * _log;

	/// count of hardware leds
	unsigned _hwLedCount;

	ComponentRegister _componentRegister;
	
	/// register of input sources and it's prio channel
	PriorityRegister _priorityRegister;

	/// flag for v4l color correction
	bool _colorAdjustmentV4Lonly;
	
	/// flag for v4l color correction
	bool _colorTransformV4Lonly;
	
	/// flag for color transform enable
	bool _transformEnabled;

	/// flag for color adjustment enable
	bool _adjustmentEnabled;

	/// flag indicates state for autoselection of input source
	bool _sourceAutoSelectEnabled;
	
	/// holds the current priority channel that is manualy selected
	int _currentSourcePriority;

	QByteArray _configHash;

	QSize _ledGridSize;
	
	int _ledMAppingType;
	
	int _configVersionId;
};
