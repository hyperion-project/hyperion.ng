
#include <grabberapp/GrabberApp.h>
#include "X11GrabberTraits.h"

int main(int argc, char** argv)
{
	return runGrabberApp<X11GrabberTraits>(argc, argv);
}
