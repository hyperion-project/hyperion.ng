#pragma once

// STL includes
#include <vector>
#include <QStringList>
#include <QString>

// Hyperion includes
#include <utils/ColorRgb.h>
#include <hyperion/ColorAdjustment.h>

///
/// The LedColorTransform is responsible for performing color transformation from 'raw' colors
/// received as input to colors mapped to match the color-properties of the LEDs.
///
class MultiColorAdjustment
{
public:
	MultiColorAdjustment(int ledCnt);
	~MultiColorAdjustment();

	/**
	 * Adds a new ColorAdjustment to this MultiColorTransform
	 *
	 * @param adjustment The new ColorAdjustment (ownership is transferred)
	 */
	void addAdjustment(ColorAdjustment * adjustment);

	void setAdjustmentForLed(const QString& id, int startLed, int endLed);

	bool verifyAdjustments() const;

	void setBacklightEnabled(bool enable);

	///
	/// Returns the identifier of all the unique ColorAdjustment
	///
	/// @return The list with unique id's of the ColorAdjustment
	QStringList getAdjustmentIds() const;

	///
	/// Returns the pointer to the ColorAdjustment with the given id
	///
	/// @param id The identifier of the ColorAdjustment
	///
	/// @return The ColorAdjustment with the given id (or nullptr if it does not exist)
	///
	ColorAdjustment* getAdjustment(const QString& id);

	///
	/// Performs the color adjustment from raw-color to led-color
	///
	/// @param ledColors The list with raw colors
	///
	void applyAdjustment(std::vector<ColorRgb>& ledColors);

private:
	/// List with transform ids
	QStringList _adjustmentIds;

	/// List with unique ColorTransforms
	std::vector<ColorAdjustment*> _adjustment;

	/// List with a pointer to the ColorAdjustment for each individual led
	std::vector<ColorAdjustment*> _ledAdjustments;

	// logger instance
	Logger * _log;
};
