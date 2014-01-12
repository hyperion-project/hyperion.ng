
// STL includes
#include <csignal>
#include <iomanip>

// QT includes
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include "V4L2Grabber.h"

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


int main(int argc, char** argv)
{
	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("Simple application to send a command to hyperion using the Json interface");
		ParameterSet & parameters = optionParser.getParameters();

		StringParameter &   argDevice             = parameters.add<StringParameter>       ('d', "device",         "The device to use [default=/dev/video0]");
		VideoStandardParameter & argVideoStandard = parameters.add<VideoStandardParameter>('v', "video-standard", "The used video standard. Valid values are PAL. NYSC, or NO-CHANGE [default=PAL]");
		IntParameter &      argInput              = parameters.add<IntParameter>          ('i', "input",          "Input channel [default=0]");
		IntParameter &      argSizeDecimation     = parameters.add<IntParameter>          ('s', "size-decimator", "Decimation factor for the output size [default=1]");
		SwitchParameter<> & argHelp               = parameters.add<SwitchParameter<> >    ('h', "help",           "Show this help message and exit");

		// set defaults
		argDevice.setDefault("/dev/video0");
		argVideoStandard.setDefault(V4L2Grabber::PAL);
		argInput.setDefault(0);
		argSizeDecimation.setDefault(1);

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
					1,
					argSizeDecimation.getValue());

		grabber.start();
		grabber.capture(1);
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
