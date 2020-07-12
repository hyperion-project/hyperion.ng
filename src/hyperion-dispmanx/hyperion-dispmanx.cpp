
// QT includes
#include <QCoreApplication>
#include <QImage>

#include <flatbufserver/FlatBufferConnection.h>
#include "DispmanxWrapper.h"

#include "HyperionConfig.h"
#include <commandline/Parser.h>

// ssdp discover
#include <ssdp/SSDPDiscover.h>

#include <utils/DefaultSignalHandler.h>

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

	DefaultSignalHandler::install();

	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("Dispmanx capture application for Hyperion. Will automatically search a Hyperion server if -a option isn't used. Please note that if you have more than one server running it's more or less random which one will be used.");

		IntOption      & argFps        = parser.add<IntOption>    ('f', "framerate",   "Capture frame rate [default: %1]", "10", 1, 25);
		IntOption      & argWidth      = parser.add<IntOption>    (0x0, "width",       "Width of the captured image [default: %1]", "64", 64);
		IntOption      & argHeight     = parser.add<IntOption>    (0x0, "height",      "Height of the captured image [default: %1]", "64", 64);

		BooleanOption  & argScreenshot = parser.add<BooleanOption>(0x0, "screenshot",   "Take a single screenshot, save it to file and quit");
		Option         & argAddress    = parser.add<Option>       ('a', "address",     "Set the address of the hyperion server [default: %1]", "127.0.0.1:19400");
		IntOption      & argPriority   = parser.add<IntOption>    ('p', "priority",    "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
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

		VideoMode videoMode = VideoMode::VIDEO_2D;

		if (parser.isSet(arg3DSBS))
		{
			videoMode = VideoMode::VIDEO_3DSBS;
		}
		else if (parser.isSet(arg3DTAB))
		{
			videoMode = VideoMode::VIDEO_3DTAB;
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
			// server searching by ssdp
			QString address = argAddress.value(parser);
			if(argAddress.value(parser) == "127.0.0.1:19400")
			{
				SSDPDiscover discover;
				address = discover.getFirstService(searchType::STY_FLATBUFSERVER);
				if(address.isEmpty())
				{
					address = argAddress.value(parser);
				}
			}
			// Create the Flabuf-connection
			FlatBufferConnection flatbuf("Dispmanx Standalone", address, argPriority.getInt(parser), parser.isSet(argSkipReply));

			// Connect the screen capturing to flatbuf connection processing
			QObject::connect(&dispmanxWrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &flatbuf, SLOT(setImage(Image<ColorRgb>)));

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
