
// QT includes
#include <QCoreApplication>
#include <QImage>

#include <protoserver/ProtoConnectionWrapper.h>
#include "DispmanxWrapper.h"

#include "HyperionConfig.h"
#include <commandline/Parser.h>

using namespace commandline;

// save the image as screenshot
void saveScreenshot(QString filename, const Image<ColorRgb> & image)
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

		IntOption      & argFps        = parser.add<IntOption>    ('f', "framerate",   "Capture frame rate [default: %1]", "10");
		IntOption      & argWidth      = parser.add<IntOption>    (0x0, "width",       "Width of the captured image [default: %1]", "64", 32, 4096);
		IntOption      & argHeight     = parser.add<IntOption>    (0x0, "height",      "Height of the captured image [default: %1]", "64", 32, 4096);

		BooleanOption  & argScreenshot = parser.add<BooleanOption>(0x0, "screenshot",   "Take a single screenshot, save it to file and quit");
		Option         & argAddress    = parser.add<Option>       ('a', "address",     "Set the address of the hyperion server [default: %1]", "127.0.0.1:19445");
		IntOption      & argPriority   = parser.add<IntOption>    ('p', "priority",    "Use the provided priority channel (the lower the number, the higher the priority) [default: %1]", "800");
		BooleanOption  & argSkipReply  = parser.add<BooleanOption>(0x0, "skip-reply",  "Do not receive and check reply messages from Hyperion");
		BooleanOption  & argHelp       = parser.add<BooleanOption>('h', "help",        "Show this help message and exit");

		IntOption      & argCropLeft   = parser.add<IntOption>    (0x0, "crop-left",   "pixels to remove on left after grabbing");
		IntOption      & argCropRight  = parser.add<IntOption>    (0x0, "crop-right",  "pixels to remove on right after grabbing");
		IntOption      & argCropTop    = parser.add<IntOption>    (0x0, "crop-top",    "pixels to remove on top after grabbing");
		IntOption      & argCropBottom = parser.add<IntOption>    (0x0, "crop-bottom", "pixels to remove on bottom after grabbing");

		BooleanOption  & arg3DSBS      = parser.add<BooleanOption>(0x0, "3DSBS",       "Interpret the incoming video stream as 3D side-by-side");
		BooleanOption  & arg3DTAB      = parser.add<BooleanOption>(0x0, "3DTAB",       "Interpret the incoming video stream as 3D top-and-bottom");

		// parse all options
		parser.process(app);

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
			parser.showHelp(1);
		}

		// Create the dispmanx grabbing stuff
		DispmanxWrapper dispmanxWrapper(
			argWidth.getInt(parser),
			argHeight.getInt(parser),
			videoMode,
			argCropLeft.getInt(parser),
			argCropRight.getInt(parser),
			argCropTop.getInt(parser),
			argCropBottom.getInt(parser),
			1000 / argFps.getInt(parser));

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> & screenshot = dispmanxWrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			// Create the Proto-connection with hyperiond
			ProtoConnectionWrapper protoWrapper(argAddress.value(parser), argPriority.getInt(parser), 1000, parser.isSet(argSkipReply));

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
