#pragma once

// QT includes
#include <QTimer>

// Bootsequence includes
#include <bootsequence/BootSequence.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

///
/// The AbstractBootSequence is an 'abstract' implementation of the BootSequence that handles the
/// event generation and Hyperion connection. Subclasses only need to specify the interval and
/// return the colors for the leds for each iteration.
///
class AbstractBootSequence : public QObject, public BootSequence
{
Q_OBJECT

public:
	///
	/// Constructs the AbstractBootSequence with the given parameters
	///
	/// @param hyperion The Hyperion instance
	/// @param interval The interval between new led colors
	/// @param iterationCnt The number of iteration performed by the boot sequence
	///
	AbstractBootSequence(Hyperion * hyperion, const int64_t interval, const unsigned iterationCnt);

	///
	/// Starts the boot-sequence
	///
	virtual void start();

protected slots:
	///
	/// Timer slot for handling each interval of the boot-sequence
	///
	void update();

protected:
	///
	/// Child-classes must implement this by returning the next led colors in the sequence
	///
	/// @return The next led colors in the boot sequence
	///
	virtual const std::vector<RgbColor>& nextColors() = 0;

private:
	/// The timer used to generate an 'update' signal every interval
	QTimer _timer;

	/// The Hyperion instance
	Hyperion * _hyperion;
	/// The priority of the boot sequence
	int _priority;

	/// The counter of the number of iterations left
	int _iterationCounter;
};

