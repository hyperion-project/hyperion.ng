#pragma once

/**
 * Enumeration of the possible video standards the grabber can be set to
 */
enum class VideoStandard {
	PAL,
	NTSC,
	SECAM,
	NO_CHANGE
};

inline VideoStandard parseVideoStandard(QString videoStandard)
{
	// convert to lower case
	videoStandard = videoStandard.toLower();

	if (videoStandard == "pal")
	{
		return VideoStandard::PAL;
	}
	else if (videoStandard == "ntsc")
	{
		return VideoStandard::NTSC;
	}
	else if (videoStandard == "secam")
	{
		return VideoStandard::SECAM;
	}

	// return the default NO_CHANGE
	return VideoStandard::NO_CHANGE;
}
