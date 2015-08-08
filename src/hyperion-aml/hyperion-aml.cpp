

// QT includes
#include <QCoreApplication>
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include "../../libsrc/grabber/amlogic/AmlogicGrabber.h"

using namespace vlofgren;

// save the image as screenshot
void saveScreenshot(const char * filename, const Image<ColorRgb> & image)
{
    // store as PNG
    QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
    pngImage.save(filename);
}

int main(int argc, char ** argv)
{
	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("X11 capture application for Hyperion");
		ParameterSet & parameters = optionParser.getParameters();

		//IntParameter           & argFps             = parameters.add<IntParameter>          ('f', "framerate",        "Capture frame rate [default=10]");
		IntParameter           & argWidth           = parameters.add<IntParameter>          (0x0, "width",       "Width of the captured image [default=128]");
		IntParameter           & argHeight          = parameters.add<IntParameter>          (0x0, "height",      "Height of the captured image [default=128]");
		SwitchParameter<>      & argScreenshot      = parameters.add<SwitchParameter<>>     (0x0, "screenshot",       "Take a single screenshot, save it to file and quit");
		StringParameter        & argAddress         = parameters.add<StringParameter>       ('a', "address",          "Set the address of the hyperion server [default: 127.0.0.1:19445]");
		IntParameter           & argPriority        = parameters.add<IntParameter>          ('p', "priority",         "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
		//SwitchParameter<>      & argSkipReply       = parameters.add<SwitchParameter<>>     (0x0, "skip-reply",       "Do not receive and check reply messages from Hyperion");
		SwitchParameter<>      & argHelp            = parameters.add<SwitchParameter<>>     ('h', "help",             "Show this help message and exit");

		// set defaults
		argWidth.setDefault(64);
		argHeight.setDefault(64);
		argAddress.setDefault("127.0.0.1:19445");
		argPriority.setDefault(800);
		
		// parse all options
		optionParser.parse(argc, const_cast<const char **>(argv));

		// check if we need to display the usage. exit if we do.
		if (argHelp.isSet())
		{
			optionParser.usage();
			return 0;
		}

		if (argScreenshot.isSet())
		{
			// Create the grabber
			AmlogicGrabber amlGrabber(argWidth.getValue(), argHeight.getValue());
			
			// Capture a single screenshot and finish
			Image<ColorRgb> screenshot;
			amlGrabber.grabFrame(screenshot);
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			// TODO[TvdZ]: Implement the proto-client mechanisme
		}

	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}
