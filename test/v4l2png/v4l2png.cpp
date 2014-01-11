
// STL includes
#include <csignal>
#include <iomanip>

// QT includes
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include "V4L2Grabber.h"

using namespace vlofgren;

int main(int argc, char** argv)
{
	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("Simple application to send a command to hyperion using the Json interface");
		ParameterSet & parameters = optionParser.getParameters();

		SwitchParameter<> & argHelp  = parameters.add<SwitchParameter<> >('h', "help",  "Show this help message and exit");

		// parse all options
		optionParser.parse(argc, const_cast<const char **>(argv));

		// check if we need to display the usage. exit if we do.
		if (argHelp.isSet())
		{
			optionParser.usage();
			return 0;
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		std::cerr << e.what() << std::endl;
		return 1;
	}

    V4L2Grabber grabber("/dev/video0", 0, V4L2Grabber::PAL, 10.0);
	grabber.start();
    grabber.capture();
    grabber.stop();

	return 0;
}
