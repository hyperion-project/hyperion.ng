#pragma once

#include <cstdint>

#include <QString>
#include <utils/Logger.h>
#include <utils/ColorRgb.h>

/// Correction for a single color byte value
/// All configuration values are unsigned int and assume the color value to be between 0 and 255
class RgbChannelAdjustment
{
public:

	/// Default constructor
	explicit RgbChannelAdjustment(const QString& channelName="");

	explicit RgbChannelAdjustment(const ColorRgb& adjust, const QString& channelName="");

	/// Constructor
	/// @param adjustR
	/// @param adjustG
	/// @param adjustB
	explicit RgbChannelAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB, const QString& channelName="");

	///
	/// Transform the given array value
	///
	/// @param input The input color bytes
	/// @param brightness The current brightness value
	/// @param red The red color component
	/// @param green The green color component
	/// @param blue The blue color component
	///
	/// @note The values are updated in place.
	///
	void apply(uint8_t input, uint8_t brightness, uint8_t & red, uint8_t & green, uint8_t & blue);

	///
	/// setAdjustment RGB
	///
	/// @param adjustR
	/// @param adjustG
	/// @param adjustB
	///
	void setAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB);
	void setAdjustment(const ColorRgb& adjust);

	/// @return The current adjustR value
	uint8_t getAdjustmentR() const;

	/// @return The current adjustG value
	uint8_t getAdjustmentG() const;

	/// @return The current adjustB value
	uint8_t getAdjustmentB() const;

private:

	struct ColorMapping {
		uint8_t red[256];
		uint8_t green[256];
		uint8_t blue[256];
	};

	/// reset init of color mapping
	void resetInitialized();

	/// Name of this channel, usefull for debug messages
	QString _channelName;

	/// Logger instance
	Logger * _log;

	/// The adjustment of RGB channel
	ColorRgb _adjust;

	/// The mapping from input color to output color
	ColorMapping _mapping;

	/// bitfield to determine white value is alreade initialized
	bool _initialized[256];

	/// current brightness value
	uint8_t _brightness;
};
