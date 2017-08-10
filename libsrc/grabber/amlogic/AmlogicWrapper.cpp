#include <grabber/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("AmLogic", &_grabber, grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(grabWidth, grabHeight)
{}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
