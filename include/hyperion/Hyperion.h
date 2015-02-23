#pragma once

// stl includes
#include <list>

// QT includes
#include <QObject>
#include <QTimer>

// hyperion-utils includes
#include <utils/Image.h>

// Hyperion includes
#include <hyperion/LedString.h>
#include <hyperion/PriorityMuxer.h>

// Effect engine includes
#include <effectengine/EffectDefinition.h>

// Forward class declaration
class LedDevice;
class ColorTransform;
class EffectEngine;
class HsvTransform;
class RgbChannelTransform;
class MultiColorTransform;

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
	/// Constructs the Hyperion instance based on the given Json configuration
	///
	/// @param[in] jsonConfig The Json configuration
	///
	Hyperion(const Json::Value& jsonConfig);

	///
	/// Destructor; cleans up resourcess
	///
	~Hyperion();

	///
	/// Returns the number of attached leds
	///
	unsigned getLedCount() const;

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

	/// Get the list of available effects
	/// @return The list of available effects
	const std::list<EffectDefinition> &getEffects() const;

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
	void setColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms, bool clearEffects = true);

	///
	/// Returns the list with unique transform identifiers
	/// @return The list with transform identifiers
	///
	const std::vector<std::string> & getTransformIds() const;

	///
	/// Returns the ColorTransform with the given identifier
	/// @return The transform with the given identifier (or nullptr if the identifier does not exist)
	///
	ColorTransform * getTransform(const std::string& id);

	/// Tell Hyperion that the transforms have changed and the leds need to be updated
	void transformsUpdated();

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
	/// @param timout The timeout of the effect (after the timout, the effect will be cleared)
	int setEffect(const std::string & effectName, int priority, int timeout = -1);

	/// Run the specified effect on the given priority channel and optionally specify a timeout
	/// @param effectName Name of the effec to run
	/// @param args arguments of the effect script
	///	@param priority The priority channel of the effect
	/// @param timout The timeout of the effect (after the timout, the effect will be cleared)
	int setEffect(const std::string & effectName, const Json::Value & args, int priority, int timeout = -1);

public:
	static ColorOrder createColorOrder(const Json::Value & deviceConfig);
	/**
	 * Construct the 'led-string' with the integration area definition per led and the color
	 * ordering of the RGB channels
	 * @param ledsConfig   The configuration of the led areas
	 * @param deviceOrder  The default RGB channel ordering
	 * @return The constructed ledstring
	 */
	static LedString createLedString(const Json::Value & ledsConfig, const ColorOrder deviceOrder);

	static MultiColorTransform * createLedColorsTransform(const unsigned ledCnt, const Json::Value & colorTransformConfig);
	static ColorTransform * createColorTransform(const Json::Value & transformConfig);
	static HsvTransform * createHsvTransform(const Json::Value & hsvConfig);
	static RgbChannelTransform * createRgbChannelTransform(const Json::Value& colorConfig);

	static LedDevice * createColorSmoothing(const Json::Value & smoothingConfig, LedDevice * ledDevice);

signals:
	/// Signal which is emitted when a priority channel is actively cleared
	/// This signal will not be emitted when a priority channel time out
	void channelCleared(int priority);

	/// Signal which is emitted when all priority channels are actively cleared
	/// This signal will not be emitted when a priority channel time out
	void allChannelsCleared();

private slots:
	///
	/// Updates the priority muxer with the current time and (re)writes the led color with applied
	/// transforms.
	///
	void update();

private:
	/// The specifiation of the led frame construction and picture integration
	LedString _ledString;

	/// The priority muxer
	PriorityMuxer _muxer;

	/// The transformation from raw colors to led colors
	MultiColorTransform * _raw2ledTransform;

	/// The actual LedDevice
	LedDevice * _device;

	/// Effect engine
	EffectEngine * _effectEngine;

	/// The timer for handling priority channel timeouts
	QTimer _timer;
};
