

// QT includes
#include <QCoreApplication>
#include <QImage>

#include <flatbufserver/FlatBufferConnection.h>
#include "OsxWrapper.h"
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
	DefaultSignalHandler::install();

	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("OSX capture application for Hyperion. Will automatically search a Hyperion server if -a option isn't used. Please note that if you have more than one server running it's more or less random which one will be used.");

		Option        & argDisplay    = parser.add<Option>       ('d', "display",    "Set the display to capture [default: %1]", "0");
		IntOption     & argFps        = parser.add<IntOption>    ('f', "framerate",  "Capture frame rate [default: %1]", "10", 1, 25);
		IntOption     & argWidth      = parser.add<IntOption>    (0x0, "width",      "Width of the captured image [default: %1]", "160", 160);
		IntOption     & argHeight     = parser.add<IntOption>    (0x0, "height",     "Height of the captured image [default: %1]", "160", 160);
		BooleanOption & argScreenshot = parser.add<BooleanOption>(0x0, "screenshot",   "Take a single screenshot, save it to file and quit");
		Option        & argAddress    = parser.add<Option>       ('a', "address",    "Set the address of the hyperion server [default: %1]", "127.0.0.1:19400");
		IntOption     & argPriority   = parser.add<IntOption>    ('p', "priority",   "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
		BooleanOption & argSkipReply  = parser.add<BooleanOption>(0x0, "skip-reply", "Do not receive and check reply messages from Hyperion");
		BooleanOption & argHelp       = parser.add<BooleanOption>('h', "help",        "Show this help message and exit");

		// parse all arguments
		parser.process(app);

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			parser.showHelp(0);
		}

		OsxWrapper osxWrapper
			(parser.isSet(argDisplay), argWidth.getInt(parser), argHeight.getInt(parser), 1000 / argFps.getInt(parser));

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> &screenshot = osxWrapper.getScreenshot();
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
			FlatBufferConnection flatbuf("OSX Standalone", address, argPriority.getInt(parser), parser.isSet(argSkipReply));

			// Connect the screen capturing to flatbuf connection processing
			QObject::connect(&osxWrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &flatbuf, SLOT(setImage(Image<ColorRgb>)));

			// Start the capturing
			osxWrapper.start();

			// Start the application
			app.exec();
        }
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		Error(Logger::getInstance("OSXGRABBER"), "%s", e.what());
		return -1;
	}
	return 0;
}
