#include <grabber/DispmanxWrapper.h>


DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("Dispmanx", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(grabWidth, grabHeight)
{
	_ggrabber = &_grabber;
}

DispmanxWrapper::~DispmanxWrapper()
{
}

void DispmanxWrapper::action()
{
	if (_grabber.grabFrame(_image) >= 0) setImage();
}
