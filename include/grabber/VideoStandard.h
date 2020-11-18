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

inline VideoStandard parseVideoStandard(const QString& videoStandard)
{
	// convert to lower case
	QString standard = videoStandard.toLower();

	if (standard == "pal")
	{
		return VideoStandard::PAL;
	}
	else if (standard == "ntsc")
	{
		return VideoStandard::NTSC;
	}
	else if (standard == "secam")
	{
		return VideoStandard::SECAM;
	}

	// return the default NO_CHANGE
	return VideoStandard::NO_CHANGE;
}
