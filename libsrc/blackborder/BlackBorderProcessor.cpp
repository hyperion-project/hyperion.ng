#include <iostream>
/*
#include <iomanip>
using std::setw;
//*/

// Blackborder includes
#include <blackborder/BlackBorderProcessor.h>


using namespace hyperion;

BlackBorderProcessor::BlackBorderProcessor(const Json::Value &blackborderConfig) :
	_unknownSwitchCnt(blackborderConfig.get("unknownFrameCnt", 600).asUInt()),
	_borderSwitchCnt(blackborderConfig.get("borderFrameCnt", 50).asUInt()),
	_maxInconsistentCnt(blackborderConfig.get("maxInconsistentCnt", 10).asUInt()),
	_blurRemoveCnt(blackborderConfig.get("blurRemoveCnt", 1).asUInt()),
	_detectionMode(blackborderConfig.get("mode", "default").asString()),
	_detector(blackborderConfig.get("threshold", 0.01).asDouble()),
	_currentBorder({true, -1, -1}),
	_previousDetectedBorder({true, -1, -1}),
	_consistentCnt(0),
	_inconsistentCnt(10)
{
	std::cout << "BLACKBORDER INFO: mode:" << _detectionMode << std::endl;
	// empty
}

BlackBorder BlackBorderProcessor::getCurrentBorder() const
{
	return _currentBorder;
}

bool BlackBorderProcessor::updateBorder(const BlackBorder & newDetectedBorder)
{
// the new changes ignore false small borders (no reset of consistance)
// as long as the previous stable state returns within 10 frames
// and will only switch to a new border if it is realy detected stable >50 frames

// sometimes the grabber delivers "bad" frames with a smaller black border (looks like random number every few frames and even when freezing the image)
// maybe some interferences of the power supply or bad signal causing this effect - not exactly sure what causes it but changing the power supply of the converter significantly increased that "random" effect on my system
// (you can check with the debug output below or if you want i can provide some output logs)
// this "random effect" caused the old algorithm to switch to that smaller border immediatly, resulting in a too small border being detected
// makes it look like the border detectionn is not working - since the new 3 line detection algorithm is more precise this became a problem specialy in dark scenes
// wisc

//	std::cout << "c: " << setw(2) << _currentBorder.verticalSize << " " << setw(2) << _currentBorder.horizontalSize << " p: " << setw(2) << _previousDetectedBorder.verticalSize << " " << setw(2) << _previousDetectedBorder.horizontalSize << " n: " << setw(2) << newDetectedBorder.verticalSize << " " << setw(2) << newDetectedBorder.horizontalSize << " c:i " << setw(2) << _consistentCnt << ":" << setw(2) << _inconsistentCnt << std::endl;

	// set the consistency counter
	if (newDetectedBorder == _previousDetectedBorder)
	{
		++_consistentCnt;
		_inconsistentCnt         = 0;
	}
	else
	{
		++_inconsistentCnt;
		if (_inconsistentCnt <= _maxInconsistentCnt)// only few inconsistent frames
		{
			//discard the newDetectedBorder -> keep the consistent count for previousDetectedBorder
			return false;
		}
		// the inconsistency threshold is reached
		// -> give the newDetectedBorder a chance to proof that its consistent
		_previousDetectedBorder = newDetectedBorder;
		_consistentCnt          = 0;
	}

	// check if there is a change
	if (_currentBorder == newDetectedBorder)
	{
		// No change required
		_inconsistentCnt = 0; // we have found a consistent border -> reset _inconsistentCnt
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
	}

	return borderChanged;
}