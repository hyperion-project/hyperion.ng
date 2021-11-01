
// QT includes
#include <QCoreApplication>
#include <QImage>

#include "QtWrapper.h"
#include <utils/ColorRgb.h>
#include <utils/Image.h>

#include "HyperionConfig.h"
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
	Logger *log = Logger::getInstance("QTGRABBER");
	Logger::setLogLevel(Logger::INFO);

	std::cout
		<< "hyperion-qt:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	//QCoreApplication app(argc, argv);
	QGuiApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("Qt interface capture application for Hyperion. Will automatically search a Hyperion server if -a option isn't used. Please note that if you have more than one server running it's more or less random which one will be used.");

		IntOption      & argDisplay         = parser.add<IntOption>    ('d', "display",        "Set the display to capture [default: %1]", "0");

		IntOption      & argFps				= parser.add<IntOption>    ('f', "framerate",      "Capture frame rate [default: %1]", QString::number(GrabberWrapper::DEFAULT_RATE_HZ), GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ, GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ);
		IntOption      & argSizeDecimation	= parser.add<IntOption>    ('s', "size-decimator", "Decimation factor for the output image size [default=%1]", QString::number(GrabberWrapper::DEFAULT_PIXELDECIMATION), 1);

		IntOption      & argCropLeft        = parser.add<IntOption>    (0x0, "crop-left",      "Number of pixels to crop from the left of the picture before decimation");
		IntOption      & argCropRight       = parser.add<IntOption>    (0x0, "crop-right",     "Number of pixels to crop from the right of the picture before decimation");
		IntOption      & argCropTop         = parser.add<IntOption>    (0x0, "crop-top",       "Number of pixels to crop from the top of the picture before decimation");
		IntOption      & argCropBottom      = parser.add<IntOption>    (0x0, "crop-bottom",    "Number of pixels to crop from the bottom of the picture before decimation");
		BooleanOption  & arg3DSBS			= parser.add<BooleanOption>(0x0, "3DSBS",          "Interpret the incoming video stream as 3D side-by-side");
		BooleanOption  & arg3DTAB			= parser.add<BooleanOption>(0x0, "3DTAB",          "Interpret the incoming video stream as 3D top-and-bottom");

		Option         & argAddress         = parser.add<Option>       ('a', "address",        "Set the address of the hyperion server [default: %1]", "127.0.0.1:19400");
		IntOption      & argPriority        = parser.add<IntOption>    ('p', "priority",       "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
		BooleanOption  & argSkipReply       = parser.add<BooleanOption>(0x0, "skip-reply",     "Do not receive and check reply messages from Hyperion");

		BooleanOption  & argScreenshot      = parser.add<BooleanOption>(0x0, "screenshot",     "Take a single screenshot, save it to file and quit");

		BooleanOption  & argDebug           = parser.add<BooleanOption>(0x0, "debug",          "Enable debug logging");
		BooleanOption  & argHelp            = parser.add<BooleanOption>('h', "help",           "Show this help message and exit");

		// parse all arguments
		parser.process(app);

		// check if debug logging is required
		if (parser.isSet(argDebug))
		{
			Logger::setLogLevel(Logger::DEBUG);
		}

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			parser.showHelp(0);
		}

		QtWrapper qtWrapper(
			argFps.getInt(parser),
			argDisplay.getInt(parser),
			argSizeDecimation.getInt(parser),
			argCropLeft.getInt(parser),
			argCropRight.getInt(parser),
			argCropTop.getInt(parser),
			argCropBottom.getInt(parser)
			);

		if (!qtWrapper.displayInit())
		{
			Error(log, "Failed to initialise the screen/display for this grabber");
			return -1;
		}

		// set 3D mode if applicable
		if (parser.isSet(arg3DSBS))
		{
			qtWrapper.setVideoMode(VideoMode::VIDEO_3DSBS);
		}
		else if (parser.isSet(arg3DTAB))
		{
			qtWrapper.setVideoMode(VideoMode::VIDEO_3DTAB);
		}

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> &screenshot = qtWrapper.getScreenshot();
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
			QObject::connect(&qtWrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &flatbuf, SLOT(setImage(Image<ColorRgb>)));

			// Start the capturing
			qtWrapper.start();

			// Start the application
			app.exec();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occurred. Display error and quit
		Error(log, "%s", e.what());
		return -1;
	}
	return 0;
}
