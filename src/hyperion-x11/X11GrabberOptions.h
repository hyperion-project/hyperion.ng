#pragma once

#include <QString>

struct X11GrabberOptions
{
	int fps = 0;
	int sizeDecimation = 0;
	int cropLeft = 0;
	int cropRight = 0;
	int cropTop = 0;
	int cropBottom = 0;

	bool video3DSBS = false;
	bool video3DTAB = false;
	bool skipReply = false;
	bool screenshot = false;
	bool debug = false;
	bool help = false;

	QString address;
	int priority = 150;
};
