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
	COMP_ALL,
	COMP_SMOOTHING,
	COMP_BLACKBORDER,
	COMP_FORWARDER,
	COMP_BOBLIGHTSERVER,
	COMP_GRABBER,
	COMP_V4L,
	COMP_COLOR,
	COMP_IMAGE,
	COMP_EFFECT,
	COMP_LEDDEVICE,
	COMP_FLATBUFSERVER,
	COMP_PROTOSERVER
};

inline const char* componentToString(Components c)
{
	switch (c)
	{
		case COMP_ALL:           return "Hyperion";
		case COMP_SMOOTHING:     return "Smoothing";
		case COMP_BLACKBORDER:   return "Blackborder detector";
		case COMP_FORWARDER:     return "Json/Proto forwarder";
		case COMP_BOBLIGHTSERVER:return "Boblight server";
		case COMP_GRABBER:       return "Framegrabber";
		case COMP_V4L:           return "V4L capture device";
		case COMP_COLOR:         return "Solid color";
		case COMP_EFFECT:        return "Effect";
		case COMP_IMAGE:         return "Image";
		case COMP_LEDDEVICE:     return "LED device";
		case COMP_FLATBUFSERVER: return "Image Receiver";
		case COMP_PROTOSERVER:   return "Proto Server";
		default:                 return "";
	}
}

inline const char* componentToIdString(Components c)
{
	switch (c)
	{
		case COMP_ALL:           return "ALL";
		case COMP_SMOOTHING:     return "SMOOTHING";
		case COMP_BLACKBORDER:   return "BLACKBORDER";
		case COMP_FORWARDER:     return "FORWARDER";
		case COMP_BOBLIGHTSERVER:return "BOBLIGHTSERVER";
		case COMP_GRABBER:       return "GRABBER";
		case COMP_V4L:           return "V4L";
		case COMP_COLOR:         return "COLOR";
		case COMP_EFFECT:        return "EFFECT";
		case COMP_IMAGE:         return "IMAGE";
		case COMP_LEDDEVICE:     return "LEDDEVICE";
		case COMP_FLATBUFSERVER: return "FLATBUFSERVER";
		case COMP_PROTOSERVER:   return "PROTOSERVER";
		default:                 return "";
	}
}

inline Components stringToComponent(const QString& component)
{
	const QString cmp = component.toUpper();
	if (cmp == "ALL")           return COMP_ALL;
	if (cmp == "SMOOTHING")     return COMP_SMOOTHING;
	if (cmp == "BLACKBORDER")   return COMP_BLACKBORDER;
	if (cmp == "FORWARDER")     return COMP_FORWARDER;
	if (cmp == "BOBLIGHTSERVER")return COMP_BOBLIGHTSERVER;
	if (cmp == "GRABBER")       return COMP_GRABBER;
	if (cmp == "V4L")           return COMP_V4L;
	if (cmp == "COLOR")         return COMP_COLOR;
	if (cmp == "EFFECT")        return COMP_EFFECT;
	if (cmp == "IMAGE")         return COMP_IMAGE;
	if (cmp == "LEDDEVICE")     return COMP_LEDDEVICE;
	if (cmp == "FLATBUFSERVER") return COMP_FLATBUFSERVER;
	if (cmp == "PROTOSERVER")   return COMP_PROTOSERVER;
	return COMP_INVALID;
}

} // end of namespace
