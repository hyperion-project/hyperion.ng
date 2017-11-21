#pragma once

/**
 * Enumeration of the possible video standards the grabber can be set to
 */
enum VideoStandard {
	VIDEOSTANDARD_PAL,
	VIDEOSTANDARD_NTSC,
	VIDEOSTANDARD_SECAM,
	VIDEOSTANDARD_NO_CHANGE
};

inline VideoStandard parseVideoStandard(QString videoStandard)
{
	// convert to lower case
	videoStandard = videoStandard.toLower();

	if (videoStandard == "pal")
	{
		return VIDEOSTANDARD_PAL;
	}
	else if (videoStandard == "ntsc")
	{
		return VIDEOSTANDARD_NTSC;
	}
	else if (videoStandard == "secam")
	{
		return VIDEOSTANDARD_SECAM;
	}

	// return the default NO_CHANGE
	return VIDEOSTANDARD_NO_CHANGE;
}
