// STL includes
#include <csignal>
#include <iomanip>
#include <clocale>

// QT includes
#include <QCoreApplication>

// blackborder includes
#include <blackborder/BlackBorderProcessor.h>

// grabber includes
#include "grabber/V4L2Grabber.h"

// proto includes
#include "protoserver/ProtoConnection.h"
#include "protoserver/ProtoConnectionWrapper.h"

// hyperion-v4l2 includes
#include "ScreenshotHandler.h"

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

int main(int argc, char** argv)
{
	std::cout
		<< "hyperion-v4l2:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	QCoreApplication app(argc, argv);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	// register the image type to use in signals
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("V4L capture application for Hyperion");

		Option             & argDevice              = parser.add<Option>       ('d', "device", "The device to use [default: %1]", "auto");
		SwitchOption<VideoStandard> & argVideoStandard= parser.add<SwitchOption<VideoStandard>>('v', "video-standard", "The used video standard. Valid values are PAL, NTSC or no-change. [default: %1]", "no-change");
		SwitchOption<PixelFormat> & argPixelFormat    = parser.add<SwitchOption<PixelFormat>>  (0x0, "pixel-format", "The use pixel format. Valid values are YUYV, UYVY, RGB32 or no-change. [default: %1]", "no-change");
		IntOption          & argInput               = parser.add<IntOption>    (0x0, "input", "Input channel (optional)", "-1");
		IntOption          & argWidth               = parser.add<IntOption>    (0x0, "width", "Try to set the width of the video input [default: %1]", "-1");
		IntOption          & argHeight              = parser.add<IntOption>    (0x0, "height", "Try to set the height of the video input [default: %1]", "-1");
		IntOption          & argCropWidth           = parser.add<IntOption>    (0x0, "crop-width", "Number of pixels to crop from the left and right sides of the picture before decimation [default: %1]", "0");
		IntOption          & argCropHeight          = parser.add<IntOption>    (0x0, "crop-height", "Number of pixels to crop from the top and the bottom of the picture before decimation [default: %1]", "0");
		IntOption          & argCropLeft            = parser.add<IntOption>    (0x0, "crop-left", "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
		IntOption          & argCropRight           = parser.add<IntOption>    (0x0, "crop-right", "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
		IntOption          & argCropTop             = parser.add<IntOption>    (0x0, "crop-top", "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
		IntOption          & argCropBottom          = parser.add<IntOption>    (0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
		IntOption          & argSizeDecimation      = parser.add<IntOption>    ('s', "size-decimator", "Decimation factor for the output size [default=%1]", "1");
		IntOption          & argFrameDecimation     = parser.add<IntOption>    ('f', "frame-decimator", "Decimation factor for the video frames [default=%1]", "1");
		BooleanOption      & argScreenshot          = parser.add<BooleanOption>(0x0, "screenshot", "Take a single screenshot, save it to file and quit");
		DoubleOption       & argSignalThreshold     = parser.add<DoubleOption> ('t', "signal-threshold", "The signal threshold for detecting the presence of a signal. Value should be between 0.0 and 1.0.", QString(), 0.0, 1.0);
		DoubleOption       & argRedSignalThreshold  = parser.add<DoubleOption> (0x0, "red-threshold", "The red signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)");
		DoubleOption       & argGreenSignalThreshold= parser.add<DoubleOption> (0x0, "green-threshold", "The green signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)");
		DoubleOption       & argBlueSignalThreshold = parser.add<DoubleOption> (0x0, "blue-threshold", "The blue signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)");
		BooleanOption      & arg3DSBS               = parser.add<BooleanOption>(0x0, "3DSBS", "Interpret the incoming video stream as 3D side-by-side");
		BooleanOption      & arg3DTAB               = parser.add<BooleanOption>(0x0, "3DTAB", "Interpret the incoming video stream as 3D top-and-bottom");
		Option             & argAddress             = parser.add<Option>       ('a', "address", "Set the address of the hyperion server [default: %1]", "127.0.0.1:19445");
		IntOption          & argPriority            = parser.add<IntOption>    ('p', "priority", "Use the provided priority channel (the lower the number, the higher the priority) [default: %1]", "800");
		BooleanOption      & argSkipReply           = parser.add<BooleanOption>(0x0, "skip-reply", "Do not receive and check reply messages from Hyperion");
		BooleanOption      & argHelp                = parser.add<BooleanOption>('h', "help", "Show this help message and exit");

		argVideoStandard.addSwitch("pal", VIDEOSTANDARD_PAL);
		argVideoStandard.addSwitch("ntsc", VIDEOSTANDARD_NTSC);
		argVideoStandard.addSwitch("no-change", VIDEOSTANDARD_NO_CHANGE);

		argPixelFormat.addSwitch("yuyv", PIXELFORMAT_YUYV);
		argPixelFormat.addSwitch("uyvy", PIXELFORMAT_UYVY);
		argPixelFormat.addSwitch("rgb32", PIXELFORMAT_RGB32);
		argPixelFormat.addSwitch("no-change", PIXELFORMAT_NO_CHANGE);

		// parse all options
		parser.process(app);

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			parser.showHelp(0);
		}

		// initialize the grabber
		V4L2Grabber grabber(
					argDevice.getStdString(parser),
					argInput.getInt(parser),
					argVideoStandard.switchValue(parser),
					argPixelFormat.switchValue(parser),
					argWidth.getInt(parser),
					argHeight.getInt(parser),
					std::max(1, argFrameDecimation.getInt(parser)),
					std::max(1, argSizeDecimation.getInt(parser)),
					std::max(1, argSizeDecimation.getInt(parser)));

		// set signal detection
		grabber.setSignalThreshold(
					std::min(1.0, std::max(0.0, parser.isSet(argRedSignalThreshold) ? argRedSignalThreshold.getDouble(parser) : argSignalThreshold.getDouble(parser))),
					std::min(1.0, std::max(0.0, parser.isSet(argGreenSignalThreshold) ? argGreenSignalThreshold.getDouble(parser) : argSignalThreshold.getDouble(parser))),
					std::min(1.0, std::max(0.0, parser.isSet(argBlueSignalThreshold) ? argBlueSignalThreshold.getDouble(parser) : argSignalThreshold.getDouble(parser))),
					50);

		// set cropping values
		grabber.setCropping(
			parser.isSet(argCropLeft) ? argCropLeft.getInt(parser) : argCropWidth.getInt(parser),
			parser.isSet(argCropRight) ? argCropRight.getInt(parser) : argCropWidth.getInt(parser),
			parser.isSet(argCropTop) ? argCropTop.getInt(parser) : argCropHeight.getInt(parser),
			parser.isSet(argCropBottom) ? argCropBottom.getInt(parser) : argCropHeight.getInt(parser));

		// set 3D mode if applicable
		if (parser.isSet(arg3DSBS))
		{
			grabber.set3D(VIDEO_3DSBS);
		}
		else if (parser.isSet(arg3DTAB))
		{
			grabber.set3D(VIDEO_3DTAB);
		}

		// run the grabber
		if (parser.isSet(argScreenshot))
		{
			ScreenshotHandler handler("screenshot.png");
			QObject::connect(&grabber, SIGNAL(newFrame(Image<ColorRgb>)), &handler, SLOT(receiveImage(Image<ColorRgb>)));
			grabber.start();
			QCoreApplication::exec();
			grabber.stop();
		}
		else
		{
			ProtoConnectionWrapper protoWrapper(argAddress.value(parser), argPriority.getInt(parser), 1000, parser.isSet(argSkipReply));
			QObject::connect(&grabber, SIGNAL(newFrame(Image<ColorRgb>)), &protoWrapper, SLOT(receiveImage(Image<ColorRgb>)));
			if (grabber.start())
				QCoreApplication::exec();
			grabber.stop();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
                Error(Logger::getInstance("V4L2GRABBER"), "%s", e.what());
		return 1;
	}

	return 0;
}
