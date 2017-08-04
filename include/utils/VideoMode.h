#pragma once

#include <QString>

/**
 * Enumeration of the possible modes in which video can be playing (2D, 3D)
 */
enum VideoMode
{
	VIDEO_2D,
	VIDEO_3DSBS,
	VIDEO_3DTAB
};

inline VideoMode parse3DMode(QString videoMode)
{
	// convert to upper case
	videoMode = videoMode.toUpper();

	if (videoMode == "3DTAB")
	{
		return VIDEO_3DTAB;
	}
	else if (videoMode == "3DSBS")
	{
		return VIDEO_3DSBS;
	}

	// return the default 2D
	return VIDEO_2D;
}

inline QString videoMode2String(VideoMode mode)
{
	switch(mode)
	{
		case VIDEO_3DTAB: return "3DTAB";
		case VIDEO_3DSBS: return "3DSBS";
		case VIDEO_2D: return "2D";
		default: return "INVALID";
	}
}
