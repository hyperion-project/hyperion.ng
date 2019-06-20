#include <grabber/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight)
	: GrabberWrapper("AmLogic", &_grabber, grabWidth, grabHeight)
	, _grabber(grabWidth, grabHeight)
{}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
