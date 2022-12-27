#include <iostream>
#include <cmath>

#include <hyperion/Hyperion.h>

// Blackborder includes
#include <blackborder/BlackBorderProcessor.h>

using namespace hyperion;

BlackBorderProcessor::BlackBorderProcessor(Hyperion* hyperion, QObject* parent)
	: QObject(parent)
	, _hyperion(hyperion)
	, _enabled(false)
	, _unknownSwitchCnt(600)
	, _borderSwitchCnt(50)
	, _maxInconsistentCnt(10)
	, _blurRemoveCnt(1)
	, _detectionMode("default")
	, _detector(nullptr)
	, _currentBorder({true, -1, -1})
	, _previousDetectedBorder({true, -1, -1})
	, _consistentCnt(0)
	, _inconsistentCnt(10)
	, _oldThreshold(-0.1)
	, _hardDisabled(false)
	, _userEnabled(false)
{
	// init
	handleSettingsUpdate(settings::BLACKBORDER, _hyperion->getSetting(settings::BLACKBORDER));

	// listen for settings updates
	connect(_hyperion, &Hyperion::settingsChanged, this, &BlackBorderProcessor::handleSettingsUpdate);

	// listen for component state changes
	connect(_hyperion, &Hyperion::compStateChangeRequest, this, &BlackBorderProcessor::handleCompStateChangeRequest);

	_detector = new BlackBorderDetector(_oldThreshold);
}

BlackBorderProcessor::~BlackBorderProcessor()
{
	delete _detector;
}

void BlackBorderProcessor::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::BLACKBORDER)
	{
		if (_hyperion->isComponentEnabled(COMP_BLACKBORDER) == -1)
		{
			//Disable, if service is not available
			_enabled = false;
			_userEnabled = false;
		}
		else
		{
			const QJsonObject& obj = config.object();
			_unknownSwitchCnt = obj["unknownFrameCnt"].toInt(600);
			_borderSwitchCnt = obj["borderFrameCnt"].toInt(50);
			_maxInconsistentCnt = obj["maxInconsistentCnt"].toInt(10);
			_blurRemoveCnt = obj["blurRemoveCnt"].toInt(1);
			_detectionMode = obj["mode"].toString("default");
			const double newThreshold = obj["threshold"].toDouble(5.0) / 100.0;

			if (fabs(_oldThreshold - newThreshold) > std::numeric_limits<double>::epsilon())
			{
				_oldThreshold = newThreshold;

				delete _detector;

				_detector = new BlackBorderDetector(newThreshold);
			}

			Debug(Logger::getInstance("BLACKBORDER", "I"+QString::number(_hyperion->getInstanceIndex())), "Set mode to: %s", QSTRING_CSTR(_detectionMode));

			// eval the comp state
			handleCompStateChangeRequest(hyperion::COMP_BLACKBORDER, obj["enable"].toBool(true));
		}
	}
}

void BlackBorderProcessor::handleCompStateChangeRequest(hyperion::Components component, bool enable)
{
	if(component == hyperion::COMP_BLACKBORDER)
	{
		_userEnabled = enable;
		if(enable)
		{
			// eg effects and probably other components don't want a BB, mimik a wrong comp state to the comp register
			if(!_hardDisabled)
				_enabled = enable;
		}
		else
		{
			_enabled = enable;
		}

		_hyperion->setNewComponentState(hyperion::COMP_BLACKBORDER, enable);
	}
}

void BlackBorderProcessor::setHardDisable(bool disable)
{
	if (disable)
	{
		_enabled = false;
	}
	else
	{
		// the user has the last word to enable
		if(_userEnabled)
			_enabled = true;
	}
	_hardDisabled = disable;
};

BlackBorder BlackBorderProcessor::getCurrentBorder() const
{
	return _currentBorder;
}

bool BlackBorderProcessor::enabled() const
{
	return _enabled;
}

void BlackBorderProcessor::setEnabled(bool enable)
{
	_enabled = enable;
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
