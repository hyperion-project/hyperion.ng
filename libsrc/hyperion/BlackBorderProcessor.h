
#pragma once

// Local Hyperion includes
#include "BlackBorderDetector.h"

class BlackBorderProcessor
{
public:
	BlackBorderProcessor();

	BlackBorder getCurrentBorder() const;

	bool process(const RgbImage& image);

private:

	const unsigned _unknownSwitchCnt;

	const unsigned _borderSwitchCnt;

	BlackBorderDetector _detector;

	BlackBorder _currentBorder;

	BlackBorder _lastDetectedBorder;

	unsigned _consistentCnt;
};

