
// STL includes
#include <csignal>

// Hyperion includes
#include <hyperionpng/HyperionPng.h>

#include "../dispmanx-helper.h"

static volatile bool sRunning = true;

void signal_handler(int signum)
{
	std::cout << "RECEIVED SIGNAL: " << signum << std::endl;
	sRunning = false;
}


int main(int /*argc*/, char** /*argv*/)
{
	// Install signal-handlers to exit the processing loop
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);

	// Construct and initialise the PNG creator with preset size
	HyperionPng hyperion;
	return dispmanx_process(hyperion, sRunning);
}
