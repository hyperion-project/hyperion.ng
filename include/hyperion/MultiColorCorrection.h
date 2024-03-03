#pragma once

// STL includes

#include <vector>

// Utils includes
#include <utils/ColorRgb.h>
#include "utils/Logger.h"

// Hyperion includes
#include <hyperion/ColorCorrection.h>

///
/// The LedColorCorrection is responsible for performing color correction from 'raw' colors
/// received as input to colors mapped to match the color-properties of the leds.
///
class MultiColorCorrection
{
public:
	MultiColorCorrection(int ledCnt);
	~MultiColorCorrection();

	/**
	 * Adds a new ColorCorrection to this MultiColorCorrection
	 *
	 * @param Correction The new ColorCorrection (ownership is transfered)
	 */
	void addCorrection(ColorCorrection * correction);

	void setCorrectionForLed(const QString& id, int startLed, int endLed);

	bool verifyCorrections() const;

	///
	/// Returns the identifier of all the unique ColorCorrection
	///
	/// @return The list with unique id's of the ColorCorrections
	QStringList & getCorrectionIds();

	///
	/// Returns the pointer to the ColorCorrection with the given id
	///
	/// @param id The identifier of the ColorCorrection
	///
	/// @return The ColorCorrection with the given id (or nullptr if it does not exist)
	///
	ColorCorrection* getCorrection(const QString& id);

	///
	/// Performs the color transoformation from raw-color to led-color
	///
	/// @param ledColors The list with raw colors
	///
	void applyCorrection(std::vector<ColorRgb>& ledColors);

private:
	/// List with Correction ids
	QStringList _correctionIds;

	/// List with unique ColorCorrections
	std::vector<ColorCorrection*> _correction;

	/// List with a pointer to the ColorCorrection for each individual led
	std::vector<ColorCorrection*> _ledCorrections;

	// logger instance
	Logger * _log;
};
