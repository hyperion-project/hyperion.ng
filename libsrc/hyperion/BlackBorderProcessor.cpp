#include "BlackBorderProcessor.h"

BlackBorderProcessor::BlackBorderProcessor() :
	_unknownSwitchCnt(600),
	_borderSwitchCnt(50),
	_detector(),
	_currentBorder({BlackBorder::unknown, 0}),
	_lastDetectedBorder({BlackBorder::unknown, 0}),
	_consistentCnt(0)
{
}

BlackBorder BlackBorderProcessor::getCurrentBorder() const
{
	return _currentBorder;
}

bool BlackBorderProcessor::process(const RgbImage& image)
{
	const BlackBorder imageBorder = _detector.process(image);

	if (imageBorder.type == _lastDetectedBorder.type && imageBorder.size == _lastDetectedBorder.size)
	{
		++_consistentCnt;
	}
	else
	{
		_lastDetectedBorder = imageBorder;
		_consistentCnt      = 0;
	}

	bool borderChanged = false;
	switch (_lastDetectedBorder.type)
	{
	case BlackBorder::none:
		borderChanged = (_currentBorder.type != BlackBorder::none);
		_currentBorder = _lastDetectedBorder;
		break;
	case BlackBorder::horizontal:
	case BlackBorder::vertical:
		if (_consistentCnt == _borderSwitchCnt)
		{
			_currentBorder = _lastDetectedBorder;
			borderChanged = true;
		}
		break;
	case BlackBorder::unknown:
		if (_consistentCnt == _unknownSwitchCnt)
		{
			_currentBorder = _lastDetectedBorder;
			borderChanged = true;
		}
		break;
	}

	return borderChanged;
}
