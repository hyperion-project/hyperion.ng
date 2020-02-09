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
	TYPE_COLOR,
	TYPE_IMAGE,
	TYPE_IMAGESTREAM,
	TYPE_EFFECT,
	COMP_LEDDEVICE,
	COMP_FLATBUFSERVER,
	COMP_PROTOSERVER
};

inline const char *componentToString(Components c)
{
	switch (c)
	{
	case COMP_ALL:
		return "Hyperion";
	case COMP_SMOOTHING:
		return "Smoothing";
	case COMP_BLACKBORDER:
		return "Blackborder detector";
	case COMP_FORWARDER:
		return "Json/Proto forwarder";
	case COMP_BOBLIGHTSERVER:
		return "Boblight server";
	case COMP_GRABBER:
		return "Framegrabber";
	case COMP_V4L:
		return "V4L capture device";
	case TYPE_COLOR:
		return "Solid color";
	case TYPE_EFFECT:
		return "Effect";
	case TYPE_IMAGE:
		return "Image";
	case TYPE_IMAGESTREAM:
		return "Image stream";
	case COMP_LEDDEVICE:
		return "LED device";
	case COMP_FLATBUFSERVER:
		return "Flatbuffer Server";
	case COMP_PROTOSERVER:
		return "Proto Server";
	default:
		return "";
	}
}

inline const char *componentToIdString(Components c)
{
	switch (c)
	{
	case COMP_ALL:
		return "ALL";
	case COMP_SMOOTHING:
		return "SMOOTHING";
	case COMP_BLACKBORDER:
		return "BLACKBORDER";
	case COMP_FORWARDER:
		return "FORWARDER";
	case COMP_BOBLIGHTSERVER:
		return "BOBLIGHTSERVER";
	case COMP_GRABBER:
		return "GRABBER";
	case COMP_V4L:
		return "V4L";
	case TYPE_COLOR:
		return "COLOR";
	case TYPE_EFFECT:
		return "EFFECT";
	case TYPE_IMAGE:
		return "IMAGE";
	case TYPE_IMAGESTREAM:
		return "IMAGESTREAM";
	case COMP_LEDDEVICE:
		return "LEDDEVICE";
	case COMP_FLATBUFSERVER:
		return "FLATBUFSERVER";
	case COMP_PROTOSERVER:
		return "PROTOSERVER";
	default:
		return "";
	}
}

inline Components stringToComponent(QString component)
{
	component = component.toUpper();
	if (component == "ALL")
		return COMP_ALL;
	if (component == "SMOOTHING")
		return COMP_SMOOTHING;
	if (component == "BLACKBORDER")
		return COMP_BLACKBORDER;
	if (component == "FORWARDER")
		return COMP_FORWARDER;
	if (component == "BOBLIGHTSERVER")
		return COMP_BOBLIGHTSERVER;
	if (component == "GRABBER")
		return COMP_GRABBER;
	if (component == "V4L")
		return COMP_V4L;
	if (component == "COLOR")
		return TYPE_COLOR;
	if (component == "EFFECT")
		return TYPE_EFFECT;
	if (component == "IMAGE")
		return TYPE_IMAGE;
	if (component == "IMAGESTREAM")
		return TYPE_IMAGESTREAM;
	if (component == "LEDDEVICE")
		return COMP_LEDDEVICE;
	if (component == "FLATBUFSERVER")
		return COMP_FLATBUFSERVER;
	if (component == "PROTOSERVER")
		return COMP_PROTOSERVER;
	return COMP_INVALID;
}

}; // namespace hyperion
