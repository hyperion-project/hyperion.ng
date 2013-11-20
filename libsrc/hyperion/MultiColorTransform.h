#pragma once

// STL includes
#include <vector>

// Utils includes
#include <utils/ColorRgb.h>

#include <utils/RgbChannelTransform.h>
#include <utils/HsvTransform.h>

struct ColorTransform
{
	std::string _id;
	/// The RED-Channel (RGB) transform
	RgbChannelTransform _rgbRedTransform;
	/// The GREEN-Channel (RGB) transform
	RgbChannelTransform _rgbGreenTransform;
	/// The BLUE-Channel (RGB) transform
	RgbChannelTransform _rgbBlueTransform;
	/// The HSV Transform for applying Saturation and Value transforms
	HsvTransform _hsvTransform;
};

///
/// The LedColorTransform is responsible for performing color transformation from 'raw' colors
/// received as input to colors mapped to match the color-properties of the leds.
///
class MultiColorTransform
{
public:
	MultiColorTransform();
	~MultiColorTransform();

	void addTransform(const std::string & id,
			const RgbChannelTransform & redTransform,
			const RgbChannelTransform & greenTransform,
			const RgbChannelTransform & blueTransform,
			const HsvTransform & hsvTransform);

	void setTransformForLed(const std::string& id, const unsigned startLed, const unsigned endLed);

	///
	/// Returns the identifier of all the unique ColorTransform
	///
	/// @return The list with unique id's of the ColorTransforms
	std::vector<std::string> getTransformIds();

	///
	/// Returns the pointer to the ColorTransform with the given id
	///
	/// @param id The identifier of the ColorTransform
	///
	/// @return The ColorTransform with the given id (or nullptr if it does not exist)
	///
	ColorTransform* getTransform(const std::string& id);

	///
	/// Performs the color transoformation from raw-color to led-color
	///
	/// @param rawColors The list with raw colors
	///
	/// @return The list with led-colors
	///
	std::vector<ColorRgb> applyTransform(const std::vector<ColorRgb>& rawColors);

private:
	/// List with unique ColorTransforms
	std::vector<ColorTransform*> _transform;

	/// List with a pointer to the ColorTransform for each individual led
	std::vector<ColorTransform*> _ledTransforms;
};
