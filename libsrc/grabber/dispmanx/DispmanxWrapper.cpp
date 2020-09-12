#include <grabber/DispmanxWrapper.h>

DispmanxWrapper::DispmanxWrapper(unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz)
	: GrabberWrapper("Dispmanx", &_grabber, grabWidth, grabHeight, updateRate_Hz)
	, _grabber(grabWidth, grabHeight)
{

}

void DispmanxWrapper::action()
{
	transferFrame(_grabber);
}
