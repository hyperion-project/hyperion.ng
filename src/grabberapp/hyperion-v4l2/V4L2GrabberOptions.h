#pragma once

#include <QString>

#include <hyperion/GrabberWrapper.h>
#include <utils/PixelFormat.h>
#include <utils/VideoStandard.h>

struct V4L2GrabberOptions
{
	QString device = QStringLiteral("auto");
	int input = 0;
	VideoStandard videoStandard = VideoStandard::NO_CHANGE;
	PixelFormat pixelFormat = PixelFormat::NO_CHANGE;
	int fps = GrabberWrapper::DEFAULT_RATE_HZ;
	FlipMode flipMode = FlipMode::NO_CHANGE;
	int width = 640;
	int height = 480;
	int sizeDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION;
	int cropLeft = 0;
	int cropRight = 0;
	int cropTop = 0;
	int cropBottom = 0;

	bool video3DSBS = false;
	bool video3DTAB = false;

	bool signalDetectionEnabled = true;
	double signalThresholdRed = 0.0;
	double signalThresholdGreen = 0.0;
	double signalThresholdBlue = 0.0;

	// Only applied when the user supplied all four --signal-[horizontal|vertical]-[min|max] options
	bool applySignalDetectionOffset = false;
	double signalHorizontalMin = 0.0;
	double signalVerticalMin = 0.0;
	double signalHorizontalMax = 0.0;
	double signalVerticalMax = 0.0;

	bool skipReply = false;
	bool screenshot = false;
	bool debug = false;
	bool help = false;
	
	QString address;
	int priority = 150;

	// Set by parseV4L2GrabberOptions() when CLI validation fails (e.g. incompatible
	// signal-area options); checked first thing in V4L2GrabberTraits::run().
	bool valid = true;
	QString error;
};
