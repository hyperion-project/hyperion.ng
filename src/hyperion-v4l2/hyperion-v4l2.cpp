
// STL includes
#include <csignal>
#include <iomanip>

// QT includes
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include "V4L2Grabber.h"
#include "ProtoConnection.h"

using namespace vlofgren;

/// Data parameter for the video standard
typedef vlofgren::PODParameter<V4L2Grabber::VideoStandard> VideoStandardParameter;

namespace vlofgren {
	/// Translates a string (as passed on the commandline) to a color standard
	///
	/// @param[in] s The string (as passed on the commandline)
	/// @return The color standard
	/// @throws Parameter::ParameterRejected If the string did not result in a video standard
	template<>
	V4L2Grabber::VideoStandard VideoStandardParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
	{
		QString input = QString::fromStdString(s).toLower();

		if (input == "pal")
		{
			return V4L2Grabber::PAL;
		}
		else if (input == "ntsc")
		{
			return V4L2Grabber::NTSC;
		}
		else if (input == "no-change")
		{
			return V4L2Grabber::NO_CHANGE;
		}

		throw Parameter::ParameterRejected("Invalid value for video standard. Valid values are: PAL, NTSC, and NO-CHANGE");
		return V4L2Grabber::NO_CHANGE;
	}
}

// save the image as screenshot
void saveScreenshot(void *, const Image<ColorRgb> & image)
{
	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save("screenshot.png");
}

// send the image to Hyperion
void sendImage(void * arg, const Image<ColorRgb> & image)
{
	ProtoConnection * connection = static_cast<ProtoConnection *>(arg);
	connection->setImage(image, 50, 200);
}

int main(int argc, char** argv)
{
	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("Simple application to send a command to hyperion using the Json interface");
		ParameterSet & parameters = optionParser.getParameters();

		StringParameter        & argDevice          = parameters.add<StringParameter>       ('d', "device",         "The device to use [default=/dev/video0]");
		VideoStandardParameter & argVideoStandard   = parameters.add<VideoStandardParameter>('v', "video-standard", "The used video standard. Valid values are PAL. NYSC, or NO-CHANGE [default=PAL]");
		IntParameter           & argInput           = parameters.add<IntParameter>          (0x0, "input",          "Input channel (optional)");
		IntParameter           & argWidth           = parameters.add<IntParameter>          (0x0, "width",          "Try to set the width of the video input (optional)");
		IntParameter           & argHeight          = parameters.add<IntParameter>          (0x0, "height",         "Try to set the height of the video input (optional)");
		IntParameter           & argCropWidth       = parameters.add<IntParameter>          (0x0, "crop-width",     "Number of pixels to crop from the left and right sides in the picture before decimation [default=0]");
		IntParameter           & argCropHeight      = parameters.add<IntParameter>          (0x0, "crop-height",    "Number of pixels to crop from the top and the bottom in the picture before decimation [default=0]");
		IntParameter           & argSizeDecimation  = parameters.add<IntParameter>          ('s', "size-decimator", "Decimation factor for the output size [default=1]");
		IntParameter           & argFrameDecimation = parameters.add<IntParameter>          ('f', "frame-decimator","Decimation factor for the video frames [default=1]");
		SwitchParameter<>      & argScreenshot      = parameters.add<SwitchParameter<>>     (0x0, "screenshot",     "Take a single screenshot, save it to file and quit");
		StringParameter        & argAddress         = parameters.add<StringParameter>       ('a', "address",        "Set the address of the hyperion server [default: 127.0.0.1:19445]");
		IntParameter           & argPriority        = parameters.add<IntParameter>          ('p', "priority",       "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
		SwitchParameter<>      & argSkipReply       = parameters.add<SwitchParameter<>>     (0x0, "skip-reply",     "Do not receive and check reply messages from Hyperion");
		SwitchParameter<>      & argHelp            = parameters.add<SwitchParameter<>>     ('h', "help",           "Show this help message and exit");

		// set defaults
		argDevice.setDefault("/dev/video0");
		argVideoStandard.setDefault(V4L2Grabber::PAL);
		argInput.setDefault(-1);
		argWidth.setDefault(-1);
		argHeight.setDefault(-1);
		argCropWidth.setDefault(0);
		argCropHeight.setDefault(0);
		argSizeDecimation.setDefault(1);
		argFrameDecimation.setDefault(1);
		argAddress.setDefault("127.0.0.1:19445");
		argPriority.setDefault(800);

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
			ProtoConnection connection(argAddress.getValue());
			connection.setSkipReply(argSkipReply.isSet());

			grabber.setCallback(&sendImage, &connection);
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
