#pragma once

#include <QString>

/**
 * Enumeration of the possible modes in which frame-grabbing is performed.
 */
enum GrabbingMode
{
	/** Frame grabbing is switched off */
	GRABBINGMODE_OFF,
	/** Frame grabbing during video */
	GRABBINGMODE_VIDEO,
	GRABBINGMODE_PAUSE,
	GRABBINGMODE_PHOTO,
	GRABBINGMODE_AUDIO,
	GRABBINGMODE_MENU,
	GRABBINGMODE_SCREENSAVER,
	GRABBINGMODE_INVALID
};

inline QString grabbingMode2String(GrabbingMode mode)
{
	switch(mode)
	{
		case GRABBINGMODE_OFF: return "OFF";
		case GRABBINGMODE_VIDEO: return "VIDEO";
		case GRABBINGMODE_PAUSE: return "PAUSE";
		case GRABBINGMODE_PHOTO: return "PHOTO";
		case GRABBINGMODE_AUDIO: return "AUDIO";
		case GRABBINGMODE_MENU: return "MENU";
		case GRABBINGMODE_SCREENSAVER: return "SCREENSAVER";
		default: return "INVALID";
	}
}
