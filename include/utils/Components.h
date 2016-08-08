#pragma once

/**
 * Enumeration of components in Hyperion.
 */
enum Components
{
	SMOOTHING,
	BLACKBORDER,
	KODICHECKER,
	FORWARDER,
	UDPLISTENER,
	BOBLIGHTSERVER,
	GRABBER
};

inline const char* componentToString(Components c)
{
	switch (c)
	{
		case SMOOTHING:     return "Smoothing option";
		case BLACKBORDER:   return "Blackborder detector";
		case KODICHECKER:   return "KodiVideoChecker";
		case FORWARDER:     return "Json/Proto forwarder";
		case UDPLISTENER:   return "UDP listener";
		case BOBLIGHTSERVER:return "Boblight server";
		case GRABBER:       return "Framegrabber";
		default:            return "";
	}
}
