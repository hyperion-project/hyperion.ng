
#pragma once

// QT includes
#include <QTimer>

// Bootsequence include
#include "AbstractBootSequence.h"

///
/// The RainborBootSequence shows a 'rainbow' (all lights have a different color). The rainbow is
/// rotated over each led during the length of the sequence.
///
class RainbowBootSequence : public AbstractBootSequence
{
public:
	///
	/// Constructs the rainbow boot-sequence. Hyperion is used for writing the led colors. The given
	/// duration is the length of the sequence.
	///
	/// @param[in] hyperion  The Hyperion instance
	/// @param[in] duration_ms  The length of the sequence [ms]
	///
	RainbowBootSequence(Hyperion * hyperion, const unsigned duration_ms);

protected:
	///
	/// Moves the rainbow one led further
	///
	const std::vector<RgbColor>& nextColors();

private:
	/// The current color of the boot sequence (the rainbow)
	std::vector<RgbColor> _ledColors;
	/// The counter of the number of iterations left
	int _iterationCounter;
};

