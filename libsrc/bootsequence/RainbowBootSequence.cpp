
// Utils includes
#include <utils/HsvTransform.h>

// Local-Bootsequence include
#include "RainbowBootSequence.h"

RainbowBootSequence::RainbowBootSequence(Hyperion * hyperion, const unsigned duration_ms) :
	AbstractBootSequence(hyperion, duration_ms/hyperion->getLedCount(), hyperion->getLedCount()),
	_ledColors(hyperion->getLedCount())
{
	for (unsigned iLed=0; iLed<hyperion->getLedCount(); ++iLed)
	{
		RgbColor& color = _ledColors[iLed];
		HsvTransform::hsv2rgb(iLed*360/hyperion->getLedCount(), 255, 255, color.red, color.green, color.blue);
	}
}

const std::vector<RgbColor>& RainbowBootSequence::nextColors()
{
	// Rotate the colors left
	const RgbColor headColor = _ledColors.front();
	for (unsigned i=1; i<_ledColors.size(); ++i)
	{
		_ledColors[i-1] = _ledColors[i];
	}
	_ledColors.back() = headColor;

	return _ledColors;
}
