
#pragma once

// Bootsequence includes
#include "AbstractBootSequence.h"

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessor.h>

///
/// The KITT BootSequence is a boot sequence inspired by the Knight Rider car: Knight Industries Two
/// Thousand (aka KITT)
///
class KittBootSequence : public AbstractBootSequence
{
public:
	///
	/// Constructs the KITT BootSequence
	///
	/// @param[in] hyperion  The Hyperion instance
	/// @param[in] duration_ms  The length of the sequence [ms]
	///
	KittBootSequence(Hyperion * hyperion, const unsigned duration_ms);

	///
	/// Destructor, deletes the processor
	///
	virtual ~KittBootSequence();

	///
	/// Returns the next led color sequence
	///
	/// @return The next colors for the leds
	///
	virtual const std::vector<RgbColor>& nextColors();

private:
	/// Image processor to compute led-colors from the image
	ImageProcessor * _processor;

	/// 1D-Image of the KITT-grill contains a single red pixel and the rest black
	RgbImage _image;

	/// The vector with led-colors
	std::vector<RgbColor> _ledColors;

	/// Direction the red-light is currently moving
	bool _forwardMove = true;
	/// The location of the current red-light
	unsigned _currentLight = 0;

	/// Moves the current light to the next (increase or decrease depending on direction)
	void moveNextLight();
};

