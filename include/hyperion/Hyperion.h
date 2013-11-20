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
	void setTransform(Transform transform, RgbChannel color, double value);

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
	double getTransform(Transform transform, RgbChannel color) const;

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

	static LedDevice * createDevice(const Json::Value & deviceConfig);
	static ColorOrder createColorOrder(const Json::Value & deviceConfig);
	static LedString createLedString(const Json::Value & ledsConfig);

	static MultiColorTransform * createLedColorsTransform(const unsigned ledCnt, const Json::Value & colorTransformConfig);
	static HsvTransform * createHsvTransform(const Json::Value & hsvConfig);
	static RgbChannelTransform * createColorTransform(const Json::Value& colorConfig);

	static LedDevice * createColorSmoothing(const Json::Value & smoothingConfig, LedDevice * ledDevice);

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

	/// Value with the desired color byte order
	ColorOrder _colorOrder;

	/// The actual LedDevice
	LedDevice * _device;

	/// The timer for handling priority channel timeouts
	QTimer _timer;
};
