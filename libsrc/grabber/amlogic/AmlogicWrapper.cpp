#include <grabber/AmlogicWrapper.h>


AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("AmLogic", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(grabWidth, grabHeight)
{
	_ggrabber = &_grabber;
}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
