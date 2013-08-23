
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
	_currentBorder({BlackBorder::unknown, 0}),
	_previousDetectedBorder({BlackBorder::unknown, 0}),
	_consistentCnt(0)
{
}

BlackBorder BlackBorderProcessor::getCurrentBorder() const
{
	if (_currentBorder.size > 0)
	{
		return {_currentBorder.type, _currentBorder.size+int(_blurRemoveCnt)};
	}

	return _currentBorder;
}

bool BlackBorderProcessor::process(const RgbImage& image)
{
	const BlackBorder imageBorder = _detector.process(image);

	if (imageBorder == _previousDetectedBorder)
	{
		++_consistentCnt;
	}
	else
	{
		_previousDetectedBorder = imageBorder;
		_consistentCnt          = 0;
	}

	if (_currentBorder == imageBorder)
	{
		// No change required
		return false;
	}

	bool borderChanged = false;
	switch (imageBorder.type)
	{
	case BlackBorder::none:
		if (_consistentCnt == 0)
		{
			_currentBorder = imageBorder;
			borderChanged = true;
		}
		break;
	case BlackBorder::horizontal:
		if (_currentBorder.type == BlackBorder::vertical || imageBorder.size < _currentBorder.size || _consistentCnt == _borderSwitchCnt)
		{
			_currentBorder = imageBorder;
			borderChanged = true;
		}
		break;
	case BlackBorder::vertical:
		if (_currentBorder.type == BlackBorder::horizontal || imageBorder.size < _currentBorder.size || _consistentCnt == _borderSwitchCnt)
		{
			_currentBorder = imageBorder;
			borderChanged = true;
		}
		break;
	case BlackBorder::unknown:
		if (_consistentCnt == _unknownSwitchCnt)
		{
			_currentBorder = imageBorder;
			borderChanged = true;
		}
		break;
	}

	return borderChanged;
}
