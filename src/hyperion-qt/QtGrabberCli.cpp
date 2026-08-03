#include "QtGrabberCli.h"

#include <commandline/BooleanOption.h>
#include <commandline/IntOption.h>
#include <commandline/Option.h>
#include <commandline/Parser.h>
#include <hyperion/GrabberWrapper.h>

using namespace commandline;

QtGrabberOptions parseQtGrabberOptions(QCoreApplication& app)
{
	Parser parser(QStringLiteral("Qt-Grabber capture application for Hyperion. Will automatically search a Hyperion server if -a option is not used. Please note that if you have more than one server running it's more or less random which one will be used."));

	IntOption& argDisplay = parser.add<IntOption>('d', "display", "Set the display to capture [default: %1]", "0");
	IntOption& argFps = parser.add<IntOption>(
		'f', "framerate",
		QString("Capture frame rate. Range %1-%2fps").arg(GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ).arg(GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ),
		QString::number(GrabberWrapper::DEFAULT_RATE_HZ),
		GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ,
		GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ);
	IntOption& argSizeDecimation = parser.add<IntOption>(
		's', "size-decimator",
		"Decimation factor for the output image size [default=%1]",
		QString::number(GrabberWrapper::DEFAULT_PIXELDECIMATION),
		1);

	IntOption& argCropLeft   = parser.add<IntOption>(0x0, "crop-left",   "Number of pixels to crop from the left of the picture before decimation");
	IntOption& argCropRight  = parser.add<IntOption>(0x0, "crop-right",  "Number of pixels to crop from the right of the picture before decimation");
	IntOption& argCropTop    = parser.add<IntOption>(0x0, "crop-top",    "Number of pixels to crop from the top of the picture before decimation");
	IntOption& argCropBottom = parser.add<IntOption>(0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation");
	BooleanOption const& arg3DSBS = parser.add<BooleanOption>(0x0, "3DSBS", "Interpret the incoming video stream as 3D side-by-side");
	BooleanOption const& arg3DTAB = parser.add<BooleanOption>(0x0, "3DTAB", "Interpret the incoming video stream as 3D top-and-bottom");

	Option const& argAddress  = parser.add<Option>('a', "address",
		"The hostname or IP-address (IPv4 or IPv6) of the hyperion server.\nDefault host: %1, port: 19400.\nSample addresses:\nHost : hyperion.fritz.box\nIPv4 : 127.0.0.1:19400\nIPv6 : [2001:1:2:3:4:5:6:7]",
		"127.0.0.1");
	IntOption& argPriority    = parser.add<IntOption>('p', "priority", "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
	BooleanOption const& argSkipReply  = parser.add<BooleanOption>(0x0, "skip-reply",  "Do not receive and check reply messages from Hyperion");
	BooleanOption const& argScreenshot = parser.add<BooleanOption>(0x0, "screenshot",  "Take a single screenshot, save it to file and quit");
	BooleanOption const& argDebug      = parser.add<BooleanOption>(0x0, "debug",       "Enable debug logging");
	BooleanOption const& argHelp       = parser.add<BooleanOption>('h', "help",        "Show this help message and exit");

	parser.process(app);

	if (parser.isSet(argHelp))
	{
		parser.showHelp(0);
	}

	QtGrabberOptions opts;
	opts.fps            = argFps.getInt(parser);
	opts.display        = argDisplay.getInt(parser);
	opts.sizeDecimation = argSizeDecimation.getInt(parser);
	opts.cropLeft       = argCropLeft.getInt(parser);
	opts.cropRight      = argCropRight.getInt(parser);
	opts.cropTop        = argCropTop.getInt(parser);
	opts.cropBottom     = argCropBottom.getInt(parser);
	opts.video3DSBS     = parser.isSet(arg3DSBS);
	opts.video3DTAB     = parser.isSet(arg3DTAB);
	opts.address        = argAddress.value(parser);
	opts.priority       = argPriority.getInt(parser);
	opts.skipReply      = parser.isSet(argSkipReply);
	opts.screenshot     = parser.isSet(argScreenshot);
	opts.debug          = parser.isSet(argDebug);
	opts.help           = parser.isSet(argHelp);
	return opts;
}
