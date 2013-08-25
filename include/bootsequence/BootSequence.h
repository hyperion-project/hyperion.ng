#pragma once

///
/// Pure virtual base class (or interface) for boot sequences. A BootSequence is started after the
/// Hyperion deamon is started to demonstrate the proper functioninf of the attached leds (and lets
/// face it because it is cool)
///
class BootSequence
{
public:

	///
	/// Empty virtual destructor for abstract base class
	///
	virtual ~BootSequence()
	{
		// empty
	}

	///
	/// Starts the boot sequence writing one or more colors to the attached leds
	///
	virtual void start() = 0;
};
