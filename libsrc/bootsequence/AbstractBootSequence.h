#pragma once

// QT includes
#include <QTimer>

// Bootsequence includes
#include <bootsequence/BootSequence.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

class AbstractBootSequence : public QObject, public BootSequence
{
Q_OBJECT

public:
	AbstractBootSequence(Hyperion * hyperion, const int64_t interval, const unsigned iterationCnt);

	virtual void start();

protected slots:
	void update();


protected:
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

