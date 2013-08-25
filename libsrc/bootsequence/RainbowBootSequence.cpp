
// Utils includes
#include <utils/HsvTransform.h>

// Local-Bootsequence include
#include "RainbowBootSequence.h"

RainbowBootSequence::RainbowBootSequence(Hyperion * hyperion, const unsigned duration_ms) :
	_timer(),
	_hyperion(hyperion),
	_priority(0),
	_ledColors(hyperion->getLedCount()),
	_iterationCounter(hyperion->getLedCount())
{
	for (unsigned iLed=0; iLed<_hyperion->getLedCount(); ++iLed)
	{
		RgbColor& color = _ledColors[iLed];
		HsvTransform::hsv2rgb(iLed*360/_hyperion->getLedCount(), 255, 255, color.red, color.green, color.blue);
	}

	_timer.setInterval(duration_ms/_hyperion->getLedCount());
	_timer.setSingleShot(false);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));
}

void RainbowBootSequence::start()
{
	_timer.start();
}

void RainbowBootSequence::update()
{
	if (_iterationCounter == 0)
	{
		_timer.stop();
		_hyperion->clear(_priority);
	}
	else
	{
		// Rotate the colors left
		const RgbColor headColor = _ledColors.front();
		for (unsigned i=1; i<_ledColors.size(); ++i)
		{
			_ledColors[i-1] = _ledColors[i];
		}
		_ledColors.back() = headColor;

		// Write the colors to hyperion
		_hyperion->setColors(_priority, _ledColors, -1);

		// Decrease the loop count
		--_iterationCounter;
	}
}
