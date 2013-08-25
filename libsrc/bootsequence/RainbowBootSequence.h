
#pragma once

// QT includes
#include <QTimer>

// Bootsequence include
#include <bootsequence/BootSequence.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

///
/// The RainborBootSequence shows a 'rainbow' (all lights have a different color). The rainbow is
/// rotated over each led during the length of the sequence.
///
class RainbowBootSequence : public QObject, public BootSequence
{
Q_OBJECT

public:
	///
	/// Constructs the rainbow boot-sequence. Hyperion is used for writing the led colors. The given
	/// duration is the length of the sequence.
	///
	/// @param[in] hyperion  The Hyperion instance
	/// @param[in] duration_ms  The length of the sequence [ms]
	///
	RainbowBootSequence(Hyperion * hyperion, const unsigned duration_ms);

	///
	/// Starts the boot-sequence
	///
	virtual void start();

private slots:
	///
	/// Moves the rainbow one led further
	///
	void update();

private:
	/// The timer used to generate an 'update' signal every interval
	QTimer _timer;

	/// The Hyperion instance
	Hyperion * _hyperion;

	/// The priority of the boot sequence
	int _priority;

	/// The current color of the boot sequence (the rainbow)
	std::vector<RgbColor> _ledColors;
	/// The counter of the number of iterations left
	int _iterationCounter;
};

