#pragma once

// STL includes
#include <vector>

// Utils includes
#include <utils/ColorRgb.h>

// Hyperion includes
#include <hyperion/ColorTransform.h>

///
/// The LedColorTransform is responsible for performing color transformation from 'raw' colors
/// received as input to colors mapped to match the color-properties of the leds.
///
class MultiColorTransform
{
public:
	MultiColorTransform(const unsigned ledCnt);
	~MultiColorTransform();

	/**
	 * Adds a new ColorTransform to this MultiColorTransform
	 *
	 * @param transform The new ColorTransform (ownership is transfered)
	 */
	void addTransform(ColorTransform * transform);

	void setTransformForLed(const std::string& id, const unsigned startLed, const unsigned endLed);

	bool verifyTransforms() const;

	///
	/// Returns the identifier of all the unique ColorTransform
	///
	/// @return The list with unique id's of the ColorTransforms
	const std::vector<std::string> & getTransformIds();

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
	/// List with transform ids
	std::vector<std::string> _transformIds;

	/// List with unique ColorTransforms
	std::vector<ColorTransform*> _transform;

	/// List with a pointer to the ColorTransform for each individual led
	std::vector<ColorTransform*> _ledTransforms;
};
