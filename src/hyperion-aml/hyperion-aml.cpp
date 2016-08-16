

// QT includes
#include <QCoreApplication>
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include <protoserver/ProtoConnectionWrapper.h>
#include "AmlogicWrapper.h"

#include "HyperionConfig.h"
#include <utils/Logger.h>


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
	std::cout
		<< "hyperion-aml:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("AmLogic capture application for Hyperion");
		ParameterSet & parameters = optionParser.getParameters();

		IntParameter           & argFps             = parameters.add<IntParameter>          ('f', "framerate",        "Capture frame rate [default: 10]");
		IntParameter           & argWidth           = parameters.add<IntParameter>          (0x0, "width",            "Width of the captured image [default: 128]");
		IntParameter           & argHeight          = parameters.add<IntParameter>          (0x0, "height",           "Height of the captured image [default: 128]");
		SwitchParameter<>      & argScreenshot      = parameters.add<SwitchParameter<>>     (0x0, "screenshot",       "Take a single screenshot, save it to file and quit");
		StringParameter        & argAddress         = parameters.add<StringParameter>       ('a', "address",          "Set the address of the hyperion server [default: 127.0.0.1:19445]");
		IntParameter           & argPriority        = parameters.add<IntParameter>          ('p', "priority",         "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
		SwitchParameter<>      & argSkipReply       = parameters.add<SwitchParameter<>>     (0x0, "skip-reply",       "Do not receive and check reply messages from Hyperion");
		SwitchParameter<>      & argHelp            = parameters.add<SwitchParameter<>>     ('h', "help",             "Show this help message and exit");

		// set defaults
		argFps.setDefault(10);
		argWidth.setDefault(160);
		argHeight.setDefault(160);
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

		int width  = argWidth.getValue();
		int height = argHeight.getValue();
		if (width < 160 || height < 160)
		{
			Warning(Logger::getInstance("AMLOGIC"), "Minimum width and height is 160");

			width  = std::max(160, width);
			height = std::max(160, height);
		}
		
		int grabInterval = 1000 / argFps.getValue();
		AmlogicWrapper amlWrapper(argWidth.getValue(),argHeight.getValue(),grabInterval);

		if (argScreenshot.isSet())
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> & screenshot = amlWrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			// Create the Proto-connection with hyperiond
			ProtoConnectionWrapper protoWrapper(argAddress.getValue(), argPriority.getValue(), 1000, argSkipReply.isSet());

			// Connect the screen capturing to the proto processing
			QObject::connect(&amlWrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &protoWrapper, SLOT(receiveImage(Image<ColorRgb>)));

			// Start the capturing
			amlWrapper.start();

			// Start the application
			app.exec();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		Error(Logger::getInstance("AMLOGIC"), "%s", e.what());
		return -1;
	}
	return 0;
}
