#include <grabber/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(unsigned grabWidth, unsigned grabHeight)
	: GrabberWrapper("AmLogic", &_grabber, grabWidth, grabHeight)
	, _grabber(grabWidth, grabHeight)
{}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
