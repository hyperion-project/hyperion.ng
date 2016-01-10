#include <iostream>
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
	_currentBorder1({true, -1, -1}),
	_currentBorder({true, -1, -1}),
	_previousDetectedBorder1({true, -1, -1}),
	_previousDetectedBorder({true, -1, -1}),
	_consistentCnt1(0),
	_consistentCnt(0),
	_inconsistentCnt1(0),
	_inconsistentCnt(0)
{
	// empty
}

BlackBorder BlackBorderProcessor::getCurrentBorder() const
{
	return _currentBorder;
}


bool BlackBorderProcessor::updateBorder(const BlackBorder & newDetectedBorder)
{
bool result1 = updateBorder1(newDetectedBorder);
if (result1)
{
	std::cout << "border change v1 " << _currentBorder1.horizontalSize << ":" << _currentBorder1.verticalSize << std::endl;
}

bool result2 = updateBorder2(newDetectedBorder);
if (result2)
{
	std::cout << "border change v2 " << _currentBorder.horizontalSize << ":" << _currentBorder.verticalSize << std::endl;
}

return result2;
}


bool BlackBorderProcessor::updateBorder1(const BlackBorder & newDetectedBorder)
{
	// set the consistency counter
	if (newDetectedBorder == _previousDetectedBorder1)
	{
		++_consistentCnt1;
	}
	else
	{
		_previousDetectedBorder1 = newDetectedBorder;
		_consistentCnt1          = 0;
	}

//	std::cout << "cur: " << _currentBorder1.verticalSize << " " << _currentBorder1.horizontalSize << " new: " << newDetectedBorder.verticalSize << " " << newDetectedBorder.horizontalSize << " c:i " << _consistentCnt1 << ":" << _inconsistentCnt1 << std::endl;

	// check if there is a change
	if (_currentBorder1 == newDetectedBorder)
	{
		// No change required
		return false;
	}

	bool borderChanged = false;
	if (newDetectedBorder.unknown)
	{
		// apply the unknown border if we consistently can't determine a border
		if (_consistentCnt1 == _unknownSwitchCnt)
		{
			_currentBorder1 = newDetectedBorder;
			borderChanged = true;
		}
	}
	else
	{
		// apply the detected border if it has been detected consistently
		if (_currentBorder1.unknown || _consistentCnt1 == _borderSwitchCnt)
		{
			_currentBorder1 = newDetectedBorder;
			borderChanged = true;
		}
		else
		{
			bool stable = (_consistentCnt >= 10) || (_inconsistentCnt >=30 );
			// apply smaller borders immediately
			if ((newDetectedBorder.verticalSize < _currentBorder1.verticalSize) && (stable))
			{
				_currentBorder1.verticalSize = newDetectedBorder.verticalSize;
				borderChanged = true;
			}

			if ((newDetectedBorder.horizontalSize < _currentBorder1.horizontalSize) && (stable))
			{
				_currentBorder1.horizontalSize = newDetectedBorder.horizontalSize;
				borderChanged = true;
			}
		}
	}

	return borderChanged;
}

bool BlackBorderProcessor::updateBorder2(const BlackBorder & newDetectedBorder)
{
	// set the consistency counter
	if (newDetectedBorder == _previousDetectedBorder)
	{
		++_consistentCnt;
		_inconsistentCnt         = 0;
	}
	else
	{
		_previousDetectedBorder = newDetectedBorder;
		_consistentCnt          = 0;
		++_inconsistentCnt;
	}

//	std::cout << "cur: " << _currentBorder.verticalSize << " " << _currentBorder.horizontalSize << " new: " << newDetectedBorder.verticalSize << " " << newDetectedBorder.horizontalSize << " c:i " << _consistentCnt << ":" << _inconsistentCnt << std::endl;

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
//		if (_consistentCnt == _unknownSwitchCnt)
		if (_consistentCnt >= _unknownSwitchCnt)
		{
			_currentBorder = newDetectedBorder;
			borderChanged = true;
		}
	}
	else
	{
		// apply the detected border if it has been detected consistently
//		if (_currentBorder.unknown || _consistentCnt == _borderSwitchCnt)
		if (_currentBorder.unknown || _consistentCnt >= _borderSwitchCnt)
		{
			_currentBorder = newDetectedBorder;
			borderChanged = true;
		}
		else
		{
			bool stable = (_consistentCnt >= 10) || (_inconsistentCnt >=30 );
										//more then A consistent seems like a new size not only a short flicker 
										//more then B inconsistent seems like the image is changing a lot and we need to set smaller border
			// apply smaller borders immediately
//			if (newDetectedBorder.verticalSize < _currentBorder.verticalSize)
			if ( (newDetectedBorder.verticalSize < _currentBorder.verticalSize) && (stable) )// almost immediatly - avoid switching for "abnormal" frames
			{
				_currentBorder.verticalSize = newDetectedBorder.verticalSize;
				borderChanged = true;
			}

//			if (newDetectedBorder.horizontalSize < _currentBorder.horizontalSize)
			if ( (newDetectedBorder.horizontalSize < _currentBorder.horizontalSize) && (stable) )
			{
				_currentBorder.horizontalSize = newDetectedBorder.horizontalSize;
				borderChanged = true;
			}
		}
	}

	return borderChanged;
}