#include <grabber/DispmanxWrapper.h>


DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("Dispmanx", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(grabWidth, grabHeight)
{
	_ggrabber = &_grabber;
	setImageProcessorEnabled(false);
}

void DispmanxWrapper::action()
{
	transferFrame(_grabber);
}
