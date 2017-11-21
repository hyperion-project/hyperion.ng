
// QT includes
#include <QCoreApplication>
#include <QImage>

#include <commandline/Parser.h>
#include "protoserver/ProtoConnectionWrapper.h"
#include "X11Wrapper.h"
#include "HyperionConfig.h"

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
		<< "hyperion-x11:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("X11 capture application for Hyperion");

		IntOption           & argFps             = parser.add<IntOption>    ('f', "framerate", "Capture frame rate [default: %1]", "10");
		BooleanOption       & argXGetImage       = parser.add<BooleanOption>('x', "xgetimage", "Use XGetImage instead of XRender");
		IntOption           & argCropWidth       = parser.add<IntOption>    (0x0, "crop-width", "Number of pixels to crop from the left and right sides of the picture before decimation [default: %1]", "0");
		IntOption           & argCropHeight      = parser.add<IntOption>    (0x0, "crop-height", "Number of pixels to crop from the top and the bottom of the picture before decimation [default: %1]", "0");
		IntOption           & argCropLeft        = parser.add<IntOption>    (0x0, "crop-left", "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
		IntOption           & argCropRight       = parser.add<IntOption>    (0x0, "crop-right", "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
		IntOption           & argCropTop         = parser.add<IntOption>    (0x0, "crop-top", "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
		IntOption           & argCropBottom      = parser.add<IntOption>    (0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
		IntOption           & argSizeDecimation  = parser.add<IntOption>    ('s', "size-decimator", "Decimation factor for the output size [default=%1]", "8");
		BooleanOption       & argScreenshot      = parser.add<BooleanOption>(0x0, "screenshot", "Take a single screenshot, save it to file and quit");
		Option              & argAddress         = parser.add<Option>       ('a', "address", "Set the address of the hyperion server [default: %1]", "127.0.0.1:19445");
		IntOption           & argPriority        = parser.add<IntOption>    ('p', "priority", "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
		BooleanOption       & argSkipReply       = parser.add<BooleanOption>(0x0, "skip-reply", "Do not receive and check reply messages from Hyperion");
		BooleanOption       & argHelp            = parser.add<BooleanOption>('h', "help", "Show this help message and exit");

		// parse all options
		parser.process(app);

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			parser.showHelp(0);
		}

		// Create the X11 grabbing stuff
		X11Wrapper x11Wrapper(
					1000 / argFps.getInt(parser),
					parser.isSet(argXGetImage),
					parser.isSet(argCropLeft) ? argCropLeft.getInt(parser) : argCropWidth.getInt(parser),
					parser.isSet(argCropRight) ? argCropRight.getInt(parser) : argCropWidth.getInt(parser),
					parser.isSet(argCropTop) ? argCropTop.getInt(parser) : argCropHeight.getInt(parser),
					parser.isSet(argCropBottom) ? argCropBottom.getInt(parser) : argCropHeight.getInt(parser),
					argSizeDecimation.getInt(parser), // horizontal decimation
					argSizeDecimation.getInt(parser)); // vertical decimation

	if (!x11Wrapper.displayInit())
	  return -1;

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> & screenshot = x11Wrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			// Create the Proto-connection with hyperiond
			ProtoConnectionWrapper protoWrapper(argAddress.value(parser), argPriority.getInt(parser), 1000, parser.isSet(argSkipReply));

			// Connect the screen capturing to the proto processing
			QObject::connect(&x11Wrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &protoWrapper, SLOT(receiveImage(Image<ColorRgb>)));

			// Connect the vodeMode to the proto processing
			QObject::connect(&protoWrapper, SIGNAL(setVideoMode(VideoMode)), &x11Wrapper, SLOT(setVideoMode(VideoMode)));

			// Start the capturing
			x11Wrapper.start();

			// Start the application
			app.exec();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		Error(Logger::getInstance("X11GRABBER"), "%s", e.what());
		return -1;
	}

	return 0;
}
