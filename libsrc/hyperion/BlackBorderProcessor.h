
#pragma once

// Local Hyperion includes
#include "BlackBorderDetector.h"

namespace hyperion
{
	class BlackBorderProcessor
	{
	public:
		BlackBorderProcessor(
				const unsigned unknownFrameCnt,
				const unsigned borderFrameCnt,
				const unsigned blurRemoveCnt);

		BlackBorder getCurrentBorder() const;

		bool process(const RgbImage& image);

	private:

		const unsigned _unknownSwitchCnt;

		const unsigned _borderSwitchCnt;

		unsigned _blurRemoveCnt;

		BlackBorderDetector _detector;

		BlackBorder _currentBorder;

		BlackBorder _lastDetectedBorder;

		unsigned _consistentCnt;
	};
} // end namespace hyperion
