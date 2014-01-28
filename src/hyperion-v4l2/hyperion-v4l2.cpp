// STL includes
#include <csignal>
#include <iomanip>
#include <clocale>

// QT includes
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

// blackborder includes
#include <blackborder/BlackBorderProcessor.h>

// hyperion-v4l2 includes
#include "V4L2Grabber.h"
#include "ProtoConnection.h"
#include "VideoStandardParameter.h"
#include "ImageHandler.h"

using namespace vlofgren;

// save the image as screenshot
void saveScreenshot(void *, const Image<ColorRgb> & image)
{
	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save("screenshot.png");
}

int main(int argc, char** argv)
{
	// force the locale
	setlocale(LC_ALL, "C");

	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("V4L capture application for Hyperion");
		ParameterSet & parameters = optionParser.getParameters();

		StringParameter        & argDevice          = parameters.add<StringParameter>       ('d', "device",           "The device to use [default=/dev/video0]");
		VideoStandardParameter & argVideoStandard   = parameters.add<VideoStandardParameter>('v', "video-standard",   "The used video standard. Valid values are PAL or NTSC (optional)");
		IntParameter           & argInput           = parameters.add<IntParameter>          (0x0, "input",            "Input channel (optional)");
		IntParameter           & argWidth           = parameters.add<IntParameter>          (0x0, "width",            "Try to set the width of the video input (optional)");
		IntParameter           & argHeight          = parameters.add<IntParameter>          (0x0, "height",           "Try to set the height of the video input (optional)");
		IntParameter           & argCropWidth       = parameters.add<IntParameter>          (0x0, "crop-width",       "Number of pixels to crop from the left and right sides in the picture before decimation [default=0]");
		IntParameter           & argCropHeight      = parameters.add<IntParameter>          (0x0, "crop-height",      "Number of pixels to crop from the top and the bottom in the picture before decimation [default=0]");
		IntParameter           & argSizeDecimation  = parameters.add<IntParameter>          ('s', "size-decimator",   "Decimation factor for the output size [default=1]");
		IntParameter           & argFrameDecimation = parameters.add<IntParameter>          ('f', "frame-decimator",  "Decimation factor for the video frames [default=1]");
		SwitchParameter<>      & argScreenshot      = parameters.add<SwitchParameter<>>     (0x0, "screenshot",       "Take a single screenshot, save it to file and quit");
		DoubleParameter        & argSignalThreshold = parameters.add<DoubleParameter>       ('t', "signal-threshold", "The signal threshold for detecting the presence of a signal. Value should be between 0.0 and 1.0.");
		StringParameter        & argAddress         = parameters.add<StringParameter>       ('a', "address",          "Set the address of the hyperion server [default: 127.0.0.1:19445]");
		IntParameter           & argPriority        = parameters.add<IntParameter>          ('p', "priority",         "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
		SwitchParameter<>      & argSkipReply       = parameters.add<SwitchParameter<>>     (0x0, "skip-reply",       "Do not receive and check reply messages from Hyperion");
		SwitchParameter<>      & argHelp            = parameters.add<SwitchParameter<>>     ('h', "help",             "Show this help message and exit");

		// set defaults
		argDevice.setDefault("/dev/video0");
		argVideoStandard.setDefault(V4L2Grabber::NO_CHANGE);
		argInput.setDefault(-1);
		argWidth.setDefault(-1);
		argHeight.setDefault(-1);
		argCropWidth.setDefault(0);
		argCropHeight.setDefault(0);
		argSizeDecimation.setDefault(1);
		argFrameDecimation.setDefault(1);
		argAddress.setDefault("127.0.0.1:19445");
		argPriority.setDefault(800);
		argSignalThreshold.setDefault(-1);

		// parse all options
		optionParser.parse(argc, const_cast<const char **>(argv));

		// check if we need to display the usage. exit if we do.
		if (argHelp.isSet())
		{
			optionParser.usage();
			return 0;
		}

		V4L2Grabber grabber(
					argDevice.getValue(),
					argInput.getValue(),
					argVideoStandard.getValue(),
					argWidth.getValue(),
					argHeight.getValue(),
					std::max(0, argCropWidth.getValue()),
					std::max(0, argCropHeight.getValue()),
					std::max(1, argFrameDecimation.getValue()),
					std::max(1, argSizeDecimation.getValue()));

		grabber.start();
		if (argScreenshot.isSet())
		{
			grabber.setCallback(&saveScreenshot, nullptr);
			grabber.capture(1);
		}
		else
		{
			ImageHandler handler(argAddress.getValue(), argPriority.getValue(), argSignalThreshold.getValue(), argSkipReply.isSet());
			grabber.setCallback(&ImageHandler::imageCallback, &handler);
			grabber.capture();
		}
		grabber.stop();
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
