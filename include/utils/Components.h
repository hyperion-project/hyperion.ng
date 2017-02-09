#pragma once
#include <QString>

namespace hyperion
{
/**
 * Enumeration of components in Hyperion.
 */
enum Components
{
	COMP_INVALID,
	COMP_SMOOTHING,
	COMP_BLACKBORDER,
	COMP_KODICHECKER,
	COMP_FORWARDER,
	COMP_UDPLISTENER,
	COMP_BOBLIGHTSERVER,
	COMP_GRABBER,
	COMP_V4L,
	COMP_COLOR,
	COMP_EFFECT
};

inline const char* componentToString(Components c)
{
	switch (c)
	{
		case COMP_SMOOTHING:     return "Smoothing";
		case COMP_BLACKBORDER:   return "Blackborder detector";
		case COMP_KODICHECKER:   return "KodiVideoChecker";
		case COMP_FORWARDER:     return "Json/Proto forwarder";
		case COMP_UDPLISTENER:   return "UDP listener";
		case COMP_BOBLIGHTSERVER:return "Boblight server";
		case COMP_GRABBER:       return "Framegrabber";
		case COMP_V4L:           return "V4L capture device";
		case COMP_COLOR:         return "Solid color";
		case COMP_EFFECT:        return "Effect";
		default:                 return "";
	}
}

inline const char* componentToIdString(Components c)
{
	switch (c)
	{
		case COMP_SMOOTHING:     return "SMOOTHING";
		case COMP_BLACKBORDER:   return "BLACKBORDER";
		case COMP_KODICHECKER:   return "KODICHECKER";
		case COMP_FORWARDER:     return "FORWARDER";
		case COMP_UDPLISTENER:   return "UDPLISTENER";
		case COMP_BOBLIGHTSERVER:return "BOBLIGHTSERVER";
		case COMP_GRABBER:       return "GRABBER";
		case COMP_V4L:           return "V4L";
		case COMP_COLOR:         return "COLOR";
		case COMP_EFFECT:        return "EFFECT";
		default:                 return "";
	}
}

inline  Components stringToComponent(QString component)
{
	component = component.toUpper();
	if (component == "SMOOTHING")     return COMP_SMOOTHING;
	if (component == "BLACKBORDER")   return COMP_BLACKBORDER;
	if (component == "KODICHECKER")   return COMP_KODICHECKER;
	if (component == "FORWARDER")     return COMP_FORWARDER;
	if (component == "UDPLISTENER")   return COMP_UDPLISTENER;
	if (component == "BOBLIGHTSERVER")return COMP_BOBLIGHTSERVER;
	if (component == "GRABBER")       return COMP_GRABBER;
	if (component == "V4L")           return COMP_V4L;
	if (component == "COLOR")         return COMP_COLOR;
	if (component == "EFFECT")        return COMP_EFFECT;

	return COMP_INVALID;
}

}
