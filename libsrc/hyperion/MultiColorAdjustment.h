#pragma once

// STL includes
#include <vector>

// Utils includes
#include <utils/ColorRgb.h>

// Hyperion includes
#include <hyperion/ColorAdjustment.h>

///
/// The LedColorTransform is responsible for performing color transformation from 'raw' colors
/// received as input to colors mapped to match the color-properties of the leds.
///
class MultiColorAdjustment
{
public:
	MultiColorAdjustment(const unsigned ledCnt);
	~MultiColorAdjustment();

	/**
	 * Adds a new ColorAdjustment to this MultiColorTransform
	 *
	 * @param adjustment The new ColorAdjustment (ownership is transfered)
	 */
	void addAdjustment(ColorAdjustment * adjustment);

	void setAdjustmentForLed(const std::string& id, const unsigned startLed, const unsigned endLed);

	bool verifyAdjustments() const;

	///
	/// Returns the identifier of all the unique ColorAdjustment
	///
	/// @return The list with unique id's of the ColorAdjustment
	const std::vector<std::string> & getAdjustmentIds();

	///
	/// Returns the pointer to the ColorAdjustment with the given id
	///
	/// @param id The identifier of the ColorAdjustment
	///
	/// @return The ColorAdjustment with the given id (or nullptr if it does not exist)
	///
	ColorAdjustment* getAdjustment(const std::string& id);

	///
	/// Performs the color adjustment from raw-color to led-color
	///
	/// @param rawColors The list with raw colors
	///
	/// @return The list with led-colors
	///
	std::vector<ColorRgb> applyAdjustment(const std::vector<ColorRgb>& rawColors);

private:
	/// List with transform ids
	std::vector<std::string> _adjustmentIds;

	/// List with unique ColorTransforms
	std::vector<ColorAdjustment*> _adjustment;

	/// List with a pointer to the ColorAdjustment for each individual led
	std::vector<ColorAdjustment*> _ledAdjustments;
};
