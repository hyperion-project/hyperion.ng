#include <grabber/DispmanxWrapper.h>

DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("Dispmanx", &_grabber, grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(grabWidth, grabHeight)
{
	setImageProcessorEnabled(false);
}

void DispmanxWrapper::action()
{
	transferFrame(_grabber);
}
