
// QT includes
#include <QCoreApplication>
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include <protoserver/ProtoConnectionWrapper.h>
#include "DispmanxWrapper.h"

#include "HyperionConfig.h"

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
		<< "hyperion-dispmanx:" << std::endl
		<< "\tversion   : " << HYPERION_VERSION_ID << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("Dispmanx capture application for Hyperion");
		ParameterSet & parameters = optionParser.getParameters();

		IntParameter           & argFps        = parameters.add<IntParameter>          ('f', "framerate",   "Capture frame rate [default: 10]");
		IntParameter           & argWidth      = parameters.add<IntParameter>          (0x0, "width",       "The width of the grabbed frames [pixels]");
		IntParameter           & argHeight     = parameters.add<IntParameter>          (0x0, "height",      "The height of the grabbed frames");

		SwitchParameter<>      & argScreenshot = parameters.add<SwitchParameter<>>     (0x0, "screenshot",  "Take a single screenshot, save it to file and quit");
		StringParameter        & argAddress    = parameters.add<StringParameter>       ('a', "address",     "Set the address of the hyperion server [default: 127.0.0.1:19445]");
		IntParameter           & argPriority   = parameters.add<IntParameter>          ('p', "priority",    "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
		SwitchParameter<>      & argSkipReply  = parameters.add<SwitchParameter<>>     (0x0, "skip-reply",  "Do not receive and check reply messages from Hyperion");
		SwitchParameter<>      & argHelp       = parameters.add<SwitchParameter<>>     ('h', "help",        "Show this help message and exit");

		IntParameter           & argCropLeft   = parameters.add<IntParameter>          (0x0, "crop-left",       "pixels to remove on left after grabbing");
		IntParameter           & argCropRight  = parameters.add<IntParameter>          (0x0, "crop-right",       "pixels to remove on right after grabbing");
		IntParameter           & argCropTop    = parameters.add<IntParameter>          (0x0, "crop-top",       "pixels to remove on top after grabbing");
		IntParameter           & argCropBottom = parameters.add<IntParameter>          (0x0, "crop-bottom",       "pixels to remove on bottom after grabbing");

		SwitchParameter<>      & arg3DSBS           = parameters.add<SwitchParameter<>>     (0x0, "3DSBS",            "Interpret the incoming video stream as 3D side-by-side");
		SwitchParameter<>      & arg3DTAB           = parameters.add<SwitchParameter<>>     (0x0, "3DTAB",            "Interpret the incoming video stream as 3D top-and-bottom");

		// set defaults
		argFps.setDefault(10);
		argWidth.setDefault(64);
		argHeight.setDefault(64);
		argAddress.setDefault("127.0.0.1:19445");
		argPriority.setDefault(800);

		argCropLeft.setDefault(0);
		argCropRight.setDefault(0);
		argCropTop.setDefault(0);
		argCropBottom.setDefault(0);

		// parse all options
		optionParser.parse(argc, const_cast<const char **>(argv));

		VideoMode videoMode = VIDEO_2D;

		if (arg3DSBS.isSet())
		{
			videoMode = VIDEO_3DSBS;
		}
		else if (arg3DTAB.isSet())
		{
			videoMode = VIDEO_3DTAB;
		}

		// check if we need to display the usage. exit if we do.
		if (argHelp.isSet())
		{
			optionParser.usage();
			return 0;
		}

		// Create the dispmanx grabbing stuff
		int grabInterval = 1000 / argFps.getValue();
		DispmanxWrapper dispmanxWrapper(argWidth.getValue(),argHeight.getValue(),
			videoMode,
			std::max(0, argCropLeft.getValue()),
			std::max(0, argCropRight.getValue()),
			std::max(0, argCropTop.getValue()),
			std::max(0, argCropBottom.getValue()),
			grabInterval);

		if (argScreenshot.isSet())
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> & screenshot = dispmanxWrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			// Create the Proto-connection with hyperiond
			ProtoConnectionWrapper protoWrapper(argAddress.getValue(), argPriority.getValue(), 1000, argSkipReply.isSet());

			// Connect the screen capturing to the proto processing
			QObject::connect(&dispmanxWrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &protoWrapper, SLOT(receiveImage(Image<ColorRgb>)));

			// Start the capturing
			dispmanxWrapper.start();

			// Start the application
			app.exec();
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
