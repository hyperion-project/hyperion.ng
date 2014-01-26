// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

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
