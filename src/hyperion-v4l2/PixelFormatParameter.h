// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

// grabber includes
#include <utils/PixelFormat.h>

using namespace vlofgren;

/// Data parameter for the pixel format
typedef vlofgren::PODParameter<PixelFormat> PixelFormatParameter;

namespace vlofgren {
    /// Translates a string (as passed on the commandline) to a pixel format
    ///
    /// @param[in] s The string (as passed on the commandline)
    /// @return The pixel format
    /// @throws Parameter::ParameterRejected If the string did not result in a pixel format
    template<>
    PixelFormat PixelFormatParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
    {
        QString input = QString::fromStdString(s).toLower();

        if (input == "yuyv")
        {
            return PIXELFORMAT_YUYV;
        }
        else if (input == "uyvy")
        {
            return PIXELFORMAT_UYVY;
        }
        else if (input == "rgb32")
        {
            return PIXELFORMAT_RGB32;
        }
        else if (input == "no-change")
        {
            return PIXELFORMAT_NO_CHANGE;
        }

        throw Parameter::ParameterRejected("Invalid value for pixel format. Valid values are: YUYV, UYVY, RGB32, and NO-CHANGE");
        return PIXELFORMAT_NO_CHANGE;
    }
}
