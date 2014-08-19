#pragma once

#include <string>
#include <algorithm>

/**
 * Enumeration of the possible video standards the grabber can be set to
 */
enum VideoStandard {
	VIDEOSTANDARD_PAL,
	VIDEOSTANDARD_NTSC,
	VIDEOSTANDARD_NO_CHANGE
};

inline VideoStandard parseVideoStandard(std::string videoStandard)
{
	// convert to lower case
	std::transform(videoStandard.begin(), videoStandard.end(), videoStandard.begin(), ::tolower);

	if (videoStandard == "pal")
	{
		return VIDEOSTANDARD_PAL;
	}
	else if (videoStandard == "ntsc")
	{
		return VIDEOSTANDARD_NTSC;
	}

	// return the default NO_CHANGE
	return VIDEOSTANDARD_NO_CHANGE;
}
