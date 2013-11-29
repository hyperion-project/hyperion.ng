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
#include <hyperion/LedDevice.h>
#include <hyperion/PriorityMuxer.h>

// Forward class declaration
class HsvTransform;
class ColorTransform;
class EffectEngine;

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
	enum Color
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

	/// Enumeration containing the possible orders of device color byte data
	enum ColorOrder
	{
		ORDER_RGB, ORDER_RBG, ORDER_GRB, ORDER_BRG, ORDER_GBR, ORDER_BGR
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
	/// Returns the value of a specific color transform
	///
	/// @param[in] transform The type of transform
	/// @param[in] color The color channel to which the transform applies (only applicable for
	///                  Transform::THRESHOLD, Transform::GAMMA, Transform::BLACKLEVEL,
	///                  Transform::WHITELEVEL)
	///
	/// @return The value of the specified color transform
	///
	double getTransform(Transform transform, Color color) const;

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
	std::list<std::string> getEffects() const;

public slots:
	///
	/// Writes a single color to all the leds for the given time and priority
	///
	/// @param[in] priority The priority of the written color
	/// @param[in] ledColor The color to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given color [ms]
	///
	void setColor(int priority, const ColorRgb &ledColor, const int timeout_ms);

	///
	/// Writes the given colors to all leds for the given time and priority
	///
	/// @param[in] priority The priority of the written colors
	/// @param[in] ledColors The colors to write to the leds
	/// @param[in] timeout_ms The time the leds are set to the given colors [ms]
	///
	void setColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms);

	///
	/// Sets/Updates a part of the color transformation.
	///
	/// @param[in] transform  The type of transform to configure
	/// @param[in] color The color channel to which the transform applies (only applicable for
	///                  Transform::THRESHOLD, Transform::GAMMA, Transform::BLACKLEVEL,
	///                  Transform::WHITELEVEL)
	/// @param[in] value  The new value for the given transform
	///
	void setTransform(Transform transform, Color color, double value);

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

public:
	static LedDevice * createDevice(const Json::Value & deviceConfig);
	static ColorOrder createColorOrder(const Json::Value & deviceConfig);
	static LedString createLedString(const Json::Value & ledsConfig);
	static HsvTransform * createHsvTransform(const Json::Value & hsvConfig);
	static ColorTransform * createColorTransform(const Json::Value & colorConfig);
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
	///
	/// Applies all color transmforms to the given list of colors. The transformation is performed
	/// in place.
	///
	/// @param colors  The colors to be transformed
	///
	void applyTransform(std::vector<ColorRgb>& colors) const;

	/// The specifiation of the led frame construction and picture integration
	LedString _ledString;

	/// The priority muxer
	PriorityMuxer _muxer;

	/// The HSV Transform for applying Saturation and Value transforms
	HsvTransform * _hsvTransform;
	/// The RED-Channel (RGB) transform
	ColorTransform * _redTransform;
	/// The GREEN-Channel (RGB) transform
	ColorTransform * _greenTransform;
	/// The BLUE-Channel (RGB) transform
	ColorTransform * _blueTransform;

	/// Value with the desired color byte order
	ColorOrder _colorOrder;

	/// The actual LedDevice
	LedDevice * _device;

	/// Effect engine
	EffectEngine * _effectEngine;

	/// The timer for handling priority channel timeouts
	QTimer _timer;
};
