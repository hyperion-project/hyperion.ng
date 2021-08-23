#pragma once

#include <QString>

/**
 * Enumeration of the possible video standards the grabber can be set to
 */
enum class VideoStandard {
	PAL,
	NTSC,
	SECAM,
	NO_CHANGE
};

inline VideoStandard parseVideoStandard(const QString& videoStandard)
{
	// convert to lower case
	QString standard = videoStandard.toUpper();

	if (standard == "PAL")
	{
		return VideoStandard::PAL;
	}
	else if (standard == "NTSC")
	{
		return VideoStandard::NTSC;
	}
	else if (standard == "SECAM")
	{
		return VideoStandard::SECAM;
	}

	// return the default NO_CHANGE
	return VideoStandard::NO_CHANGE;
}

inline QString VideoStandard2String(VideoStandard videoStandard)
{
	switch (videoStandard)
	{
		case VideoStandard::PAL: return "PAL";
		case VideoStandard::NTSC: return "NTSC";
		case VideoStandard::SECAM: return "SECAM";
		default: return "NO_CHANGE";
	}
}
