// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

// grabber includes
#include <grabber/VideoStandard.h>

using namespace vlofgren;

/// Data parameter for the video standard
typedef vlofgren::PODParameter<VideoStandard> VideoStandardParameter;

namespace vlofgren {
	/// Translates a string (as passed on the commandline) to a color standard
	///
	/// @param[in] s The string (as passed on the commandline)
	/// @return The color standard
	/// @throws Parameter::ParameterRejected If the string did not result in a video standard
	template<>
	VideoStandard VideoStandardParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
	{
		QString input = QString::fromStdString(s).toLower();

		if (input == "pal")
		{
			return VIDEOSTANDARD_PAL;
		}
		else if (input == "ntsc")
		{
			return VIDEOSTANDARD_NTSC;
		}
		else if (input == "no-change")
		{
			return VIDEOSTANDARD_NO_CHANGE;
		}

		throw Parameter::ParameterRejected("Invalid value for video standard. Valid values are: PAL, NTSC, and NO-CHANGE");
		return VIDEOSTANDARD_NO_CHANGE;
	}
}
