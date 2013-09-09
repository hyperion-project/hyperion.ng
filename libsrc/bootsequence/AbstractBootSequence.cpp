#include "AbstractBootSequence.h"

AbstractBootSequence::AbstractBootSequence(Hyperion * hyperion, const int64_t interval, const unsigned iterationCnt) :
	_timer(),
	_hyperion(hyperion),
	_priority(0),
	_iterationCounter(iterationCnt)
{
	_timer.setInterval(interval);
	_timer.setSingleShot(false);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));
}

void AbstractBootSequence::start()
{
	_timer.start();
}

void AbstractBootSequence::update()
{
	if (_iterationCounter == 0)
	{
		_timer.stop();
		_hyperion->clear(_priority);
		return;
	}

	// Obtain the next led-colors from the child-class
	const std::vector<RgbColor>& colors = nextColors();
	// Write the colors to hyperion
	_hyperion->setColors(_priority, colors, -1);

	// Decrease the loop count
	--_iterationCounter;
}
