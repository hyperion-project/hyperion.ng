
// QT includes
#include <QCoreApplication>
#include <QImage>

#include "QtWrapper.h"
#include <utils/ColorRgb.h>
#include <utils/Image.h>
#include <commandline/Parser.h>

//flatbuf sending
#include <flatbufserver/FlatBufferConnection.h>

// ssdp discover
#include <ssdp/SSDPDiscover.h>

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
	//QCoreApplication app(argc, argv);
	QGuiApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("Qt interface capture application for Hyperion");

		Option        & argDisplay         = parser.add<Option>       ('d', "display",    "Set the display to capture [default: %1]","0");
		IntOption     & argFps             = parser.add<IntOption>    ('f', "framerate",  "Capture frame rate [default: %1]", "10", 1, 25);
		IntOption     & argCropLeft        = parser.add<IntOption>    (0x0, "crop-left", "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
		IntOption     & argCropRight       = parser.add<IntOption>    (0x0, "crop-right", "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
		IntOption     & argCropTop         = parser.add<IntOption>    (0x0, "crop-top", "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
		IntOption     & argCropBottom      = parser.add<IntOption>    (0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
		IntOption     & argSizeDecimation  = parser.add<IntOption>    ('s', "size-decimator", "Decimation factor for the output image size [default=%1]", "8", 1);
		BooleanOption & argScreenshot      = parser.add<BooleanOption>(0x0, "screenshot",   "Take a single screenshot, save it to file and quit");
		Option        & argAddress         = parser.add<Option>       ('a', "address",    "Set the address of the hyperion server [default: %1]", "127.0.0.1:19400");
		IntOption     & argPriority        = parser.add<IntOption>    ('p', "priority",   "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
		BooleanOption & argSkipReply       = parser.add<BooleanOption>(0x0, "skip-reply", "Do not receive and check reply messages from Hyperion");
		BooleanOption & argHelp            = parser.add<BooleanOption>('h', "help",        "Show this help message and exit");

		// parse all arguments
		parser.process(app);

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			parser.showHelp(0);
		}

		QtWrapper grabber(
			1000 / argFps.getInt(parser),
			argCropLeft.getInt(parser),
			argCropRight.getInt(parser),
			argCropTop.getInt(parser),
			argCropBottom.getInt(parser),
			argSizeDecimation.getInt(parser),
			parser.isSet(argDisplay));

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> &screenshot = grabber.getScreenshot();
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
			FlatBufferConnection flatbuf("Qt Standalone", address, argPriority.getInt(parser), parser.isSet(argSkipReply));

			// Connect the screen capturing to flatbuf connection processing
			QObject::connect(&grabber, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &flatbuf, SLOT(setImage(Image<ColorRgb>)));

			// Start the capturing
			grabber.start();

			// Start the application
			app.exec();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		Error(Logger::getInstance("QTGRABBER"), "%s", e.what());
		return -1;
	}
	return 0;
}
