#include <grabber/DispmanxWrapper.h>

DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz)
	: GrabberWrapper("Dispmanx", &_grabber, grabWidth, grabHeight, updateRate_Hz)
	, _grabber(grabWidth, grabHeight)
{
	setImageProcessorEnabled(false);
}

void DispmanxWrapper::action()
{
	transferFrame(_grabber);
}
