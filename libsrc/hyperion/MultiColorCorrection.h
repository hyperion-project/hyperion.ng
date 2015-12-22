#pragma once

// STL includes
#include <vector>

// Utils includes
#include <utils/ColorRgb.h>

// Hyperion includes
#include <hyperion/ColorCorrection.h>

///
/// The LedColorTransform is responsible for performing color transformation from 'raw' colors
/// received as input to colors mapped to match the color-properties of the leds.
///
class MultiColorCorrection
{
public:
	MultiColorCorrection(const unsigned ledCnt);
	~MultiColorCorrection();

	/**
	 * Adds a new ColorTransform to this MultiColorTransform
	 *
	 * @param transform The new ColorTransform (ownership is transfered)
	 */
	void addCorrection(ColorCorrection * correction);

	void setCorrectionForLed(const std::string& id, const unsigned startLed, const unsigned endLed);

	bool verifyCorrections() const;

	///
	/// Returns the identifier of all the unique ColorTransform
	///
	/// @return The list with unique id's of the ColorTransforms
	const std::vector<std::string> & getCorrectionIds();

	///
	/// Returns the pointer to the ColorTransform with the given id
	///
	/// @param id The identifier of the ColorTransform
	///
	/// @return The ColorTransform with the given id (or nullptr if it does not exist)
	///
	ColorCorrection* getCorrection(const std::string& id);

	///
	/// Performs the color transoformation from raw-color to led-color
	///
	/// @param rawColors The list with raw colors
	///
	/// @return The list with led-colors
	///
	std::vector<ColorRgb> applyCorrection(const std::vector<ColorRgb>& rawColors);

private:
	/// List with transform ids
	std::vector<std::string> _correctionIds;

	/// List with unique ColorTransforms
	std::vector<ColorCorrection*> _correction;

	/// List with a pointer to the ColorTransform for each individual led
	std::vector<ColorCorrection*> _ledCorrections;
};
