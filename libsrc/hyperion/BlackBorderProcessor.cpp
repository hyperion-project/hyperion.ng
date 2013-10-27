
// Local-Hyperion includes
#include "BlackBorderProcessor.h"

using namespace hyperion;

BlackBorderProcessor::BlackBorderProcessor(
		const unsigned unknownFrameCnt,
		const unsigned borderFrameCnt,
		const unsigned blurRemoveCnt) :
	_unknownSwitchCnt(unknownFrameCnt),
	_borderSwitchCnt(borderFrameCnt),
	_blurRemoveCnt(blurRemoveCnt),
	_detector(),
	_currentBorder({true, -1, -1}),
	_previousDetectedBorder({true, -1, -1}),
	_consistentCnt(0)
{
}

BlackBorder BlackBorderProcessor::getCurrentBorder() const
{
	return _currentBorder;
}

bool BlackBorderProcessor::process(const RgbImage& image)
{
	// get the border for the single image
	BlackBorder imageBorder = _detector.process(image);

	// add blur to the border
	if (imageBorder.horizontalSize > 0)
	{
		imageBorder.horizontalSize += _blurRemoveCnt;
	}
	if (imageBorder.verticalSize > 0)
	{
		imageBorder.verticalSize += _blurRemoveCnt;
	}

	// set the consistency counter
	if (imageBorder == _previousDetectedBorder)
	{
		++_consistentCnt;
	}
	else
	{
		_previousDetectedBorder = imageBorder;
		_consistentCnt          = 0;
	}

	// check if there is a change
	if (_currentBorder == imageBorder)
	{
		// No change required
		return false;
	}

	bool borderChanged = false;
	if (imageBorder.unknown)
	{
		// apply the unknown border if we consistently can't determine a border
		if (_consistentCnt == _unknownSwitchCnt)
		{
			_currentBorder = imageBorder;
			borderChanged = true;
		}
	}
	else
	{
		// apply the detected border if it has been detected consistently
		if (_currentBorder.unknown || _consistentCnt == _borderSwitchCnt)
		{
			_currentBorder = imageBorder;
			borderChanged = true;
		}
		else
		{
			// apply smaller borders immediately
			if (imageBorder.verticalSize < _currentBorder.verticalSize)
			{
				_currentBorder.verticalSize = imageBorder.verticalSize;
				borderChanged = true;
			}

			if (imageBorder.horizontalSize < _currentBorder.horizontalSize)
			{
				_currentBorder.horizontalSize = imageBorder.horizontalSize;
				borderChanged = true;
			}
		}
	}

	return borderChanged;
}
