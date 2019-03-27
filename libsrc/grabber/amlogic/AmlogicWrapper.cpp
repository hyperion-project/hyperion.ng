#include <grabber/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz)
	: GrabberWrapper("AmLogic", &_grabber, grabWidth, grabHeight, updateRate_Hz)
	, _grabber(grabWidth, grabHeight)
{}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
