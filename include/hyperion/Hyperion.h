#pragma once

// stl includes
#include <list>

// QT includes
#include <QString>
#include <QStringList>
#include <QSize>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QMap>
#include <QMutex>

// hyperion-utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/VideoMode.h>

// Hyperion includes
#include <hyperion/LedString.h>
#include <hyperion/PriorityMuxer.h>
#include <hyperion/ColorAdjustment.h>
#include <hyperion/ComponentRegister.h>

// Effect engine includes
#include <effectengine/EffectDefinition.h>
#include <effectengine/ActiveEffectDefinition.h>
#include <effectengine/EffectSchema.h>

#include <leddevice/LedDevice.h>

// settings utils
#include <utils/settings.h>

// Forward class declaration
class HyperionDaemon;
class ImageProcessor;
class MessageForwarder;
class LinearColorSmoothing;
class EffectEngine;
class MultiColorAdjustment;
class ColorAdjustment;
class SettingsManager;
class BGEffectHandler;
class CaptureCont;
class BoblightServer;
class LedDeviceWrapper;

///
/// The main class of Hyperion. This gives other 'users' access to the attached LedDevice through
/// the priority muxer.
///
class Hyperion : public QObject
{
	Q_OBJECT
public:
	///  Type definition of the info structure used by the priority muxer
	using InputInfo = PriorityMuxer::InputInfo;

	///
	/// Destructor; cleans up resources
	///
	~Hyperion() override;

	///
	/// free all alocated objects, should be called only from constructor or before restarting hyperion
	///
	void freeObjects(bool emitCloseSignal=false);

	ImageProcessor* getImageProcessor() { return _imageProcessor; }

	///
	/// @brief Get instance index of this instance
	/// @return The index of this instance
	///
	const quint8 & getInstanceIndex() { return _instIndex; }

	///
	/// @brief Return the size of led grid
	///
	QSize getLedGridSize() const { return _ledGridSize; }

	/// gets the methode how image is maped to leds
	const int & getLedMappingType();

	/// forward smoothing config
	unsigned addSmoothingConfig(int settlingTime_ms, double ledUpdateFrequency_hz=25.0, unsigned updateDelay=0);
	unsigned updateSmoothingConfig(unsigned id, int settlingTime_ms=200, double ledUpdateFrequency_hz=25.0, unsigned updateDelay=0);


	const VideoMode & getCurrentVideoMode();

	///
	/// @brief Get the current active led device
	/// @return The device name
	///
	const QString & getActiveDeviceType();

	///
	/// @brief Get pointer to current LedDevice
	///
	LedDevice * getActiveDevice() const;

public slots:
	int getLatchTime() const;

	///
	/// Returns the number of attached leds
	///
	unsigned getLedCount() const;

	///
	/// @brief  Register a new input by priority, the priority is not active (timeout -100 isn't muxer recognized) until you start to update the data with setInput()
	/// 		A repeated call to update the base data of a known priority won't overwrite their current timeout
	/// @param[in] priority    The priority of the channel
	/// @param[in] component   The component of the channel
	/// @param[in] origin      Who set the channel (CustomString@IP)
	/// @param[in] owner       Specific owner string, might be empty
	/// @param[in] smooth_cfg  The smooth id to use
	///
	void registerInput(const int priority, const hyperion::Components& component, const QString& origin = "System", const QString& owner = "", unsigned smooth_cfg = 0);

	///
	/// @brief   Update the current color of a priority (prev registered with registerInput())
	///  		 DO NOT use this together with setInputImage() at the same time!
	/// @param  priority     The priority to update
	/// @param  ledColors    The colors
	/// @param  timeout_ms   The new timeout (defaults to -1 endless)
	/// @param  clearEffect  Should be true when NOT called from an effect
	/// @return              True on success, false when priority is not found
	///
	bool setInput(const int priority, const std::vector<ColorRgb>& ledColors, const int timeout_ms = -1, const bool& clearEffect = true);

	///
	/// @brief   Update the current image of a priority (prev registered with registerInput())
	/// 		 DO NOT use this together with setInput() at the same time!
	/// @param  priority     The priority to update
	/// @param  image        The new image
	/// @param  timeout_ms   The new timeout (defaults to -1 endless)
	/// @param  clearEffect  Should be true when NOT called from an effect
	/// @return              True on success, false when priority is not found
	///
	bool setInputImage(const int priority, const Image<ColorRgb>& image, const int64_t timeout_ms = -1, const bool& clearEffect = true);

	///
	/// Writes a single color to all the leds for the given time and priority
	/// Registers comp color or provided type against muxer
	/// Should be never used to update leds continuous
	///
	/// @param[in] priority The priority of the written color
	/// @param[in] ledColors The color to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given color [ms]
	/// @param[in] origin   The setter
	/// @param     clearEffect  Should be true when NOT called from an effect
	///
	void setColor(const int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms = -1, const QString& origin = "System" ,bool clearEffects = true);

	///
	/// @brief Set the given priority to inactive
	/// @param priority  The priority
	/// @return True on success false if not found
	///
	bool setInputInactive(const quint8& priority);

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

	/// Tell Hyperion that the corrections have changed and the leds need to be updated
	void adjustmentsUpdated();

	///
	/// Clears the given priority channel. This will switch the led-colors to the colors of the next
	/// lower priority channel (or off if no more channels are set)
	///
	/// @param[in] priority  The priority channel. -1 clears all priorities
	/// @param[in] forceClearAll Force the clear
	/// @return              True on success else false (not found)
	///
	bool clear(const int priority, bool forceClearAll=false);

	/// #############
	// EFFECTENGINE
	///
	/// @brief Get a pointer to the effect engine
	/// @return     EffectEngine instance pointer
	///

	EffectEngine* getEffectEngineInstance() { return _effectEngine; }
	///
	/// @brief Save an effect
	/// @param  obj  The effect args
	/// @return Empty on success else error message
	///
	QString saveEffect(const QJsonObject& obj);

	///
	/// @brief Delete an effect by name.
	/// @param  effectName  The effect name to delete
	/// @return Empty on success else error message
	///
	QString deleteEffect(const QString& effectName);

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
	int setEffect(const QString &effectName
				, const QJsonObject &args
				, int priority
				, int timeout = -1
				, const QString &pythonScript = ""
				, const QString &origin="System"
				, const QString &imageData = ""
	);

	/// Get the list of available effects
	/// @return The list of available effects
	const std::list<EffectDefinition> &getEffects() const;

	/// Get the list of active effects
	/// @return The list of active effects
	const std::list<ActiveEffectDefinition> &getActiveEffects();

	/// Get the list of available effect schema files
	/// @return The list of available effect schema files
	const std::list<EffectSchema> &getEffectSchemas();

	/// #############
	/// PRIORITYMUXER
	///
	/// @brief Get a pointer to the priorityMuxer instance
	/// @return      PriorityMuxer instance pointer
	///
	PriorityMuxer* getMuxerInstance() { return &_muxer; }

	///
	/// @brief enable/disable automatic/priorized source selection
	/// @param state The new state
	///
	void setSourceAutoSelect(const bool state);

	///
	/// @brief set current input source to visible
	/// @param priority the priority channel which should be vidible
	/// @return true if success, false on error
	///
	bool setVisiblePriority(const int& priority);

	/// gets current state of automatic/priorized source selection
	/// @return the state
	bool sourceAutoSelectEnabled();

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
	/// Returns a list of all registered priorities
	///
	/// @return The list with priorities
	///
	QList<int> getActivePriorities() const;

	///
	/// Returns the information of a specific priorrity channel
	///
	/// @param[in] priority  The priority channel
	///
	/// @return The information of the given, a not found priority will return lowest priority as fallback
	///
	const InputInfo getPriorityInfo(const int priority) const;

	/// #############
	/// SETTINGSMANAGER
	///
	/// @brief Get a setting by settings::type from SettingsManager
	/// @param type  The settingsType from enum
	/// @return      Data Document
	///
	QJsonDocument getSetting(const settings::type& type);

	/// gets the current json config object from SettingsManager
	/// @return json config
	const QJsonObject& getQJsonConfig();

	///
	/// @brief Save a complete json config
	/// @param config  The entire config object
	/// @param correct If true will correct json against schema before save
	/// @return        True on success else false
	///
	bool saveSettings(QJsonObject config, const bool& correct = false);

	/// ############
	/// COMPONENTREGISTER
	///
	/// @brief Get the component Register
	/// return Component register pointer
	///
	ComponentRegister& getComponentRegister() { return _componentRegister; }

	///
	/// @brief Called from components to update their current state. DO NOT CALL FROM USERS
	/// @param[in] component The component from enum
	/// @param[in] state The state of the component [true | false]
	///
	void setNewComponentState(const hyperion::Components& component, const bool& state);

	///
	/// @brief Get a list of all contrable components and their current state
	/// @return list of components
	///
	std::map<hyperion::Components, bool> getAllComponents();

	///
	/// @brief Test if a component is enabled
	/// @param The component to test
	/// @return Component state
	///
	int isComponentEnabled(const hyperion::Components& comp);

	/// sets the methode how image is maped to leds at ImageProcessor
	void setLedMappingType(const int& mappingType);

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(const VideoMode& mode);

	///
	/// @brief Init after thread start
	///
	void start();

	///
	/// @brief Stop the execution of this thread, helper to properly track eventing
	///
	void stop();

signals:
	/// Signal which is emitted when a priority channel is actively cleared
	/// This signal will not be emitted when a priority channel time out
	void channelCleared(int priority);

	/// Signal which is emitted when all priority channels are actively cleared
	/// This signal will not be emitted when a priority channel time out
	void allChannelsCleared();

	///
	/// @brief Emits whenever a user request a component state change, it's up the component to listen
	/// 	   and update the component state at the componentRegister
	/// @param component  The component from enum
	/// @param enabled    The new state of the component
	///
	void compStateChangeRequest(const hyperion::Components component, bool enabled);

	///
	/// @brief Emits whenever the imageToLedsMapping has changed
	/// @param mappingType The new mapping type
	///
	void imageToLedsMappingChanged(const int& mappingType);

	///
	/// @brief Emits whenever the visible priority delivers a image which is applied in update()
	/// 	   priorities with ledColors won't emit this signal
	/// @param  image  The current image
	///
	void currentImage(const Image<ColorRgb> & image);

	void closing();

	/// Signal which is emitted, when a new json message should be forwarded
	void forwardJsonMessage(QJsonObject);

	/// Signal which is emitted, when a new system proto image should be forwarded
	void forwardSystemProtoMessage(const QString, const Image<ColorRgb>);

	/// Signal which is emitted, when a new V4l proto image should be forwarded
	void forwardV4lProtoMessage(const QString, const Image<ColorRgb>);

	///
	/// @brief Is emitted from clients who request a videoMode change
	///
	void videoMode(const VideoMode& mode);

	///
	/// @brief A new videoMode was requested (called from Daemon!)
	///
	void newVideoMode(const VideoMode& mode);

	///
	/// @brief Emits whenever a config part changed. SIGNAL PIPE helper for SettingsManager -> HyperionDaemon
	/// @param type   The settings type from enum
	/// @param data   The data as QJsonDocument
	///
	void settingsChanged(const settings::type& type, const QJsonDocument& data);

	///
	/// @brief Emits whenever the adjustments have been updated
	///
	void adjustmentChanged();

	///
	/// @brief Signal pipe from EffectEngine to external, emits when effect list has been updated
	///
	void effectListUpdated();

	///
	/// @brief Emits whenever new data should be pushed to the LedDeviceWrapper which forwards it to the threaded LedDevice
	///
	void ledDeviceData(const std::vector<ColorRgb>& ledValues);

	///
	/// @brief Emits whenever new untransformed ledColos data is available, reflects the current visible device
	///
	void rawLedColors(const std::vector<ColorRgb>& ledValues);

	///
	/// @brief Emits before thread quit is requested
	///
	void finished();

	///
	/// @brief Emits after thread has been started
	///
	void started();

public slots:
	///
	/// Updates the priority muxer with the current time and (re)writes the led color with applied
	/// transforms.
	///
	void update();

private slots:

	///
	///	@brief Handle whenever the visible component changed
	///	@param comp      The new component
	///
	void handleVisibleComponentChanged(const hyperion::Components& comp);

	///
	///	@brief Apply settings updates for LEDS and COLOR
	///	@param type   The type from enum
	///	@param config The configuration
	///
	void handleSettingsUpdate(const settings::type& type, const QJsonDocument& config);

	///
	/// @brief Apply new videoMode from Daemon to _currVideoMode
	///
	void handleNewVideoMode(const VideoMode& mode) { _currVideoMode = mode; }

private:
	friend class HyperionDaemon;
	friend class HyperionIManager;

	///
	/// @brief Constructs the Hyperion instance, just accessible for HyperionIManager
	/// @param  instance  The instance index
	///
	Hyperion(const quint8& instance);

	/// instance index
	const quint8 _instIndex;

	/// Settings manager of this instance
	SettingsManager* _settingsManager;

	/// Register that holds component states
	ComponentRegister _componentRegister;

	/// The specifiation of the led frame construction and picture integration
	LedString _ledString;

	/// Image Processor
	ImageProcessor* _imageProcessor;

	std::vector<ColorOrder> _ledStringColorOrder;

	/// The priority muxer
	PriorityMuxer _muxer;

	/// The adjustment from raw colors to led colors
	MultiColorAdjustment * _raw2ledAdjustment;

	/// The actual LedDeviceWrapper
	LedDeviceWrapper* _ledDeviceWrapper;

	/// The smoothing LedDevice
	LinearColorSmoothing * _deviceSmooth;

	/// Effect engine
	EffectEngine * _effectEngine;

	// Message forwarder
	MessageForwarder * _messageForwarder;

	/// Logger instance
	Logger * _log;

	/// count of hardware leds
	unsigned _hwLedCount;

	QSize _ledGridSize;

	/// Background effect instance, kept active to react on setting changes
	BGEffectHandler* _BGEffectHandler;
	/// Capture control for Daemon native capture
	CaptureCont* _captureCont;

	/// buffer for leds (with adjustment)
	std::vector<ColorRgb> _ledBuffer;

	VideoMode _currVideoMode = VideoMode::VIDEO_2D;

	/// Boblight instance
	BoblightServer* _boblightServer;

	QMutex _changes;
};
