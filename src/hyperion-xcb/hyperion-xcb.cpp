#include <grabberapp/GrabberApp.h>
#include "XcbGrabberTraits.h"

int main(int argc, char** argv)
{
	return runGrabberApp<XcbGrabberTraits>(argc, argv);
}
