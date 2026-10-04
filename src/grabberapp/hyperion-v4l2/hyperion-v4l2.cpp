#include <GrabberApp.h>
#include "V4L2GrabberTraits.h"

int main(int argc, char** argv)
{
	return runGrabberApp<V4L2GrabberTraits>(argc, argv);
}
