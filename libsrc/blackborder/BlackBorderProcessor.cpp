
// Blackborder includes
#include <blackborder/BlackBorderProcessor.h>

using namespace hyperion;

BlackBorderProcessor::BlackBorderProcessor(const unsigned unknownFrameCnt,
		const unsigned borderFrameCnt,
		const unsigned blurRemoveCnt,
		uint8_t blackborderThreshold) :
	_unknownSwitchCnt(unknownFrameCnt),
	_borderSwitchCnt(borderFrameCnt),
	_blurRemoveCnt(blurRemoveCnt),
	_detector(blackborderThreshold),
	_currentBorder({true, -1, -1}),
	_previousDetectedBorder({true, -1, -1}),
	_consistentCnt(0)
{
	// empty
}

BlackBorder BlackBorderProcessor::getCurrentBorder() const
{
	return _currentBorder;
}

bool BlackBorderProcessor::updateBorder(const BlackBorder & newDetectedBorder)
{
	// set the consistency counter
	if (newDetectedBorder == _previousDetectedBorder)
	{
		++_consistentCnt;
	}
	else
	{
		_previousDetectedBorder = newDetectedBorder;
		_consistentCnt          = 0;
	}

	// check if there is a change
	if (_currentBorder == newDetectedBorder)
	{
		// No change required
		return false;
	}

	bool borderChanged = false;
	if (newDetectedBorder.unknown)
	{
		// apply the unknown border if we consistently can't determine a border
		if (_consistentCnt == _unknownSwitchCnt)
		{
			_currentBorder = newDetectedBorder;
			borderChanged = true;
		}
	}
	else
	{
		// apply the detected border if it has been detected consistently
		if (_currentBorder.unknown || _consistentCnt == _borderSwitchCnt)
		{
			_currentBorder = newDetectedBorder;
			borderChanged = true;
		}
		else
		{
			// apply smaller borders immediately
			if (newDetectedBorder.verticalSize < _currentBorder.verticalSize)
			{
				_currentBorder.verticalSize = newDetectedBorder.verticalSize;
				borderChanged = true;
			}

			if (newDetectedBorder.horizontalSize < _currentBorder.horizontalSize)
			{
				_currentBorder.horizontalSize = newDetectedBorder.horizontalSize;
				borderChanged = true;
			}
		}
	}

	return borderChanged;
}
