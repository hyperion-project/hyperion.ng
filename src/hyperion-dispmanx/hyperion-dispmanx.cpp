
// QT includes
#include <QCoreApplication>
#include <QImage>

#include <protoserver/ProtoConnectionWrapper.h>
#include "DispmanxWrapper.h"

#include "HyperionConfig.h"

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
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("Dispmanx capture application for Hyperion");
		ParameterSet & parameters = optionParser.getParameters();

		IntOption           & argFps        = parser.add<IntOption>          ('f', "framerate",   "Capture frame rate [default: 10]");
		IntOption           & argWidth      = parser.add<IntOption>          (0x0, "width",       "The width of the grabbed frames [pixels]");
		IntOption           & argHeight     = parser.add<IntOption>          (0x0, "height",      "The height of the grabbed frames");

		Option      & argScreenshot = parser.add<Option>     (0x0, "screenshot",  "Take a single screenshot, save it to file and quit");
		Option        & argAddress    = parser.add<Option>       ('a', "address",     "Set the address of the hyperion server [default: 127.0.0.1:19445]");
		IntOption           & argPriority   = parser.add<IntOption>          ('p', "priority",    "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
		Option      & argSkipReply  = parser.add<Option>     (0x0, "skip-reply",  "Do not receive and check reply messages from Hyperion");
		Option      & argHelp       = parser.add<Option>     ('h', "help",        "Show this help message and exit");

		IntOption           & argCropLeft   = parser.add<IntOption>          (0x0, "crop-left",       "pixels to remove on left after grabbing");
		IntOption           & argCropRight  = parser.add<IntOption>          (0x0, "crop-right",       "pixels to remove on right after grabbing");
		IntOption           & argCropTop    = parser.add<IntOption>          (0x0, "crop-top",       "pixels to remove on top after grabbing");
		IntOption           & argCropBottom = parser.add<IntOption>          (0x0, "crop-bottom",       "pixels to remove on bottom after grabbing");

		Option      & arg3DSBS           = parser.add<Option>     (0x0, "3DSBS",            "Interpret the incoming video stream as 3D side-by-side");
		Option      & arg3DTAB           = parser.add<Option>     (0x0, "3DTAB",            "Interpret the incoming video stream as 3D top-and-bottom");

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

		if (parser.isSet(arg3DSBS))
		{
			videoMode = VIDEO_3DSBS;
		}
		else if (parser.isSet(arg3DTAB))
		{
			videoMode = VIDEO_3DTAB;
		}

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
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

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> & screenshot = dispmanxWrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			// Create the Proto-connection with hyperiond
			ProtoConnectionWrapper protoWrapper(argAddress.getValue(), argPriority.getValue(), 1000, parser.isSet(argSkipReply));

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
                Error(Logger::getInstance("DISPMANXGRABBER"), "%s", e.what());
		return -1;
	}

	return 0;
}
