#pragma once

#include <QString>

/**
 * Enumeration of the possible modes in which video can be playing (2D, 3D)
 */
enum class VideoMode
{
	VIDEO_2D,
	VIDEO_3DSBS,
	VIDEO_3DTAB
};

inline VideoMode parse3DMode(const QString& videoMode)
{
	// convert to upper case
	const QString vm = videoMode.toUpper();

	if (vm == "3DTAB")
	{
		return VideoMode::VIDEO_3DTAB;
	}
	else if (vm == "3DSBS")
	{
		return VideoMode::VIDEO_3DSBS;
	}

	// return the default 2D
	return VideoMode::VIDEO_2D;
}

inline QString videoMode2String(VideoMode mode)
{
	switch(mode)
	{
		case VideoMode::VIDEO_3DTAB: return "3DTAB";
		case VideoMode::VIDEO_3DSBS: return "3DSBS";
		case VideoMode::VIDEO_2D: return "2D";
		default: return "INVALID";
	}
}
