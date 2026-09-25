#pragma once

#include <QString>

struct AmlogicGrabberOptions
{
	int fps = 0;
	int sizeDecimation = 0;

	bool video3DSBS = false;
	bool video3DTAB = false;
	bool skipReply = false;
	bool screenshot = false;
	bool debug = false;
	bool help = false;

	QString address;
	int priority = 150;
};
