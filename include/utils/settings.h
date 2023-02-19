#pragma once
#include <QString>
#include <QJsonDocument>

///
/// @brief Provide util methods to work with SettingsManager class
///
namespace settings {
	// all available settings sections
	enum type  {
		BGEFFECT,
		FGEFFECT,
		BLACKBORDER,
		BOBLSERVER,
		COLOR,
		DEVICE,
		EFFECTS,
		NETFORWARD,
		SYSTEMCAPTURE,
		GENERAL,
		V4L2,
		AUDIO,
		JSONSERVER,
		LEDCONFIG,
		LEDS,
		LOGGER,
		SMOOTHING,
		WEBSERVER,
		INSTCAPTURE,
		NETWORK,
		FLATBUFSERVER,
		PROTOSERVER,
		INVALID
	};

	///
	/// @brief Convert settings::type to string representation
	/// @param  type  The settings::type from enum
	/// @return       The settings type as string
	///
	inline QString typeToString(type type)
	{
		switch (type)
		{
			case BGEFFECT:      return "backgroundEffect";
			case FGEFFECT:      return "foregroundEffect";
			case BLACKBORDER:   return "blackborderdetector";
			case BOBLSERVER:    return "boblightServer";
			case COLOR:         return "color";
			case DEVICE:        return "device";
			case EFFECTS:       return "effects";
			case NETFORWARD:    return "forwarder";
			case SYSTEMCAPTURE: return "framegrabber";
			case GENERAL:       return "general";
			case V4L2:          return "grabberV4L2";
			case AUDIO:			return "grabberAudio";
			case JSONSERVER:    return "jsonServer";
			case LEDCONFIG:     return "ledConfig";
			case LEDS:          return "leds";
			case LOGGER:        return "logger";
			case SMOOTHING:     return "smoothing";
			case WEBSERVER:     return "webConfig";
			case INSTCAPTURE:   return "instCapture";
			case NETWORK:       return "network";
			case FLATBUFSERVER: return "flatbufServer";
			case PROTOSERVER:   return "protoServer";
			default:            return "invalid";
		}
	}

	///
	/// @brief Convert string to settings::type representation
	/// @param  type  The string to convert
	/// @return       The settings type from enum
	///
	inline type stringToType(const QString& type)
	{
		if      (type == "backgroundEffect")     return BGEFFECT;
		else if (type == "foregroundEffect")     return FGEFFECT;
		else if (type == "blackborderdetector")  return BLACKBORDER;
		else if (type == "boblightServer")       return BOBLSERVER;
		else if (type == "color")                return COLOR;
		else if (type == "device")               return DEVICE;
		else if (type == "effects")              return EFFECTS;
		else if (type == "forwarder")            return NETFORWARD;
		else if (type == "framegrabber")         return SYSTEMCAPTURE;
		else if (type == "general")              return GENERAL;
		else if (type == "grabberV4L2")          return V4L2;
		else if (type == "grabberAudio")		 return AUDIO;
		else if (type == "jsonServer")           return JSONSERVER;
		else if (type == "ledConfig")            return LEDCONFIG;
		else if (type == "leds")                 return LEDS;
		else if (type == "logger")               return LOGGER;
		else if (type == "smoothing")            return SMOOTHING;
		else if (type == "webConfig")            return WEBSERVER;
		else if (type == "instCapture")          return INSTCAPTURE;
		else if (type == "network")              return NETWORK;
		else if (type == "flatbufServer")        return FLATBUFSERVER;
		else if (type == "protoServer")          return PROTOSERVER;
		else                                     return INVALID;
	}
}
