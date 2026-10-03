#include "V4L2GrabberCli.h"

#include <algorithm>

#include <QDebug>
#include <QLoggingCategory>

#include <commandline/BooleanOption.h>
#include <commandline/DoubleOption.h>
#include <commandline/IntOption.h>
#include <commandline/Option.h>
#include <commandline/Parser.h>
#include <commandline/SwitchOption.h>
#include <hyperion/GrabberWrapper.h>

using namespace commandline;

V4L2GrabberOptions parseV4L2GrabberOptions(const QCoreApplication& app)
{
	Parser parser(QStringLiteral("V42L-Grabber capture application for Hyperion. Will automatically search a Hyperion server if -a option is not used. Please note that if you have more than one server running it's more or less random which one will be used."));

	Option const& argDevice = parser.add<Option>('d', "device", "The device to use, can be /dev/video0 [default: %1 (auto detected)]", "auto");
	IntOption& argInput = parser.add<IntOption>('i', "input", "The device input [default: %1]", "0");
	SwitchOption<VideoStandard>& argVideoStandard = parser.add<SwitchOption<VideoStandard>>('v', "video-standard", "The used video standard. Valid values are PAL, NTSC, SECAM or no-change. [default: %1]", "no-change");
	SwitchOption<PixelFormat>& argPixelFormat = parser.add<SwitchOption<PixelFormat>>(0x0, "pixel-format", "The use pixel format. Valid values are YUYV, UYVY, RGB32, MJPEG or no-change. [default: %1]", "no-change");
	IntOption& argFps = parser.add<IntOption>('f', "framerate", QString("Capture frame rate. Range %1-%2fps").arg(GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ).arg(GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ), QString::number(GrabberWrapper::DEFAULT_RATE_HZ), GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ, GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ);

	SwitchOption<FlipMode>& argFlipMode = parser.add<SwitchOption<FlipMode>>(0x0, "flip-mode", "The used image flip mode. Valid values are HORIZONTAL, VERTICAL, BOTH or no-change. [default: %1]", "no-change");
	IntOption& argWidth = parser.add<IntOption>('w', "width", "Width of the captured image [default: %1]", "640", 640);
	IntOption& argHeight = parser.add<IntOption>('h', "height", "Height of the captured image [default: %1]", "480", 480);
	IntOption& argSizeDecimation = parser.add<IntOption>('s', "size-decimator", "Decimation factor for the output image size [default=%1]", QString::number(GrabberWrapper::DEFAULT_PIXELDECIMATION), 1);
	IntOption& argCropWidth = parser.add<IntOption>(0x0, "crop-width", "Number of pixels to crop from the left and right sides of the picture before decimation [default: %1]", "0");
	IntOption& argCropHeight = parser.add<IntOption>(0x0, "crop-height", "Number of pixels to crop from the top and the bottom of the picture before decimation [default: %1]", "0");
	IntOption& argCropLeft = parser.add<IntOption>(0x0, "crop-left", "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
	IntOption& argCropRight = parser.add<IntOption>(0x0, "crop-right", "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
	IntOption& argCropTop = parser.add<IntOption>(0x0, "crop-top", "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
	IntOption& argCropBottom = parser.add<IntOption>(0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
	BooleanOption const& arg3DSBS = parser.add<BooleanOption>(0x0, "3DSBS", "Interpret the incoming video stream as 3D side-by-side");
	BooleanOption const& arg3DTAB = parser.add<BooleanOption>(0x0, "3DTAB", "Interpret the incoming video stream as 3D top-and-bottom");

	BooleanOption const& argSignalDetection = parser.add<BooleanOption>(0x0, "signal-detection-disabled", "disable signal detection");
	DoubleOption& argSignalThreshold = parser.add<DoubleOption>(0x0, "signal-threshold", "The signal threshold for detecting the presence of a signal. Value should be between 0.0 and 1.0.", QString(), 0.0, 1.0);
	DoubleOption& argRedSignalThreshold = parser.add<DoubleOption>(0x0, "red-threshold", "The red signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)", QString(), 0.0, 1.0);
	DoubleOption& argGreenSignalThreshold = parser.add<DoubleOption>(0x0, "green-threshold", "The green signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)", QString(), 0.0, 1.0);
	DoubleOption& argBlueSignalThreshold = parser.add<DoubleOption>(0x0, "blue-threshold", "The blue signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)", QString(), 0.0, 1.0);
	DoubleOption& argSignalHorizontalMin = parser.add<DoubleOption>(0x0, "signal-horizontal-min", "area for signal detection - horizontal minimum offset value. Values between 0.0 and 1.0", QString(), 0.0, 1.0);
	DoubleOption& argSignalVerticalMin = parser.add<DoubleOption>(0x0, "signal-vertical-min", "area for signal detection - vertical minimum offset value. Values between 0.0 and 1.0", QString(), 0.0, 1.0);
	DoubleOption& argSignalHorizontalMax = parser.add<DoubleOption>(0x0, "signal-horizontal-max", "area for signal detection - horizontal maximum offset value. Values between 0.0 and 1.0", QString(), 0.0, 1.0);
	DoubleOption& argSignalVerticalMax = parser.add<DoubleOption>(0x0, "signal-vertical-max", "area for signal detection - vertical maximum offset value. Values between 0.0 and 1.0", QString(), 0.0, 1.0);

	Option const& argAddress = parser.add<Option>('a', "address", "The hostname or IP-address (IPv4 or IPv6) of the hyperion server.\nDefault host: %1, port: 19400.\nSample addresses:\nHost : hyperion.fritz.box\nIPv4 : 127.0.0.1:19400\nIPv6 : [2001:1:2:3:4:5:6:7]", "127.0.0.1");
	IntOption& argPriority = parser.add<IntOption>('p', "priority", "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
	BooleanOption const& argSkipReply = parser.add<BooleanOption>(0x0, "skip-reply", "Do not receive and check reply messages from Hyperion");

	BooleanOption const& argScreenshot = parser.add<BooleanOption>('S', "screenshot", "Take a single screenshot, save it to file and quit");

	BooleanOption const& argDebug = parser.add<BooleanOption>(0x0, "debug", "Enable debug logging");
	// Note: unlike the other standalone grabbers, "help" has no short option here since
	// '-h' is already used for "height" above.
	BooleanOption const& argHelp = parser.add<BooleanOption>(0x0, "help", "Show this help message and exit");

	argVideoStandard.addSwitch("pal", VideoStandard::PAL);
	argVideoStandard.addSwitch("ntsc", VideoStandard::NTSC);
	argVideoStandard.addSwitch("secam", VideoStandard::SECAM);
	argVideoStandard.addSwitch("no-change", VideoStandard::NO_CHANGE);

	argPixelFormat.addSwitch("yuyv", PixelFormat::YUYV);
	argPixelFormat.addSwitch("uyvy", PixelFormat::UYVY);
	argPixelFormat.addSwitch("rgb32", PixelFormat::RGB32);
#ifdef HAVE_JPEG
	argPixelFormat.addSwitch("mjpeg", PixelFormat::MJPEG);
#endif
	argPixelFormat.addSwitch("no-change", PixelFormat::NO_CHANGE);

	argFlipMode.addSwitch("horizontal", FlipMode::HORIZONTAL);
	argFlipMode.addSwitch("vertical", FlipMode::VERTICAL);
	argFlipMode.addSwitch("both", FlipMode::BOTH);
	argFlipMode.addSwitch("no-change", FlipMode::NO_CHANGE);

	// parse all options
	parser.process(app);

	// check if we need to display the usage. exit if we do.
	if (parser.isSet(argHelp))
	{
		parser.showHelp(0);
	}

	V4L2GrabberOptions opts;
	opts.device         = argDevice.value(parser);
	opts.input          = argInput.getInt(parser);
	opts.videoStandard  = argVideoStandard.switchValue(parser);
	opts.pixelFormat    = argPixelFormat.switchValue(parser);
	opts.fps            = argFps.getInt(parser);
	opts.flipMode       = argFlipMode.switchValue(parser);
	opts.width          = argWidth.getInt(parser);
	opts.height         = argHeight.getInt(parser);
	opts.sizeDecimation = std::max(1, argSizeDecimation.getInt(parser));

	opts.cropLeft   = parser.isSet(argCropLeft)   ? argCropLeft.getInt(parser)   : argCropWidth.getInt(parser);
	opts.cropRight  = parser.isSet(argCropRight)  ? argCropRight.getInt(parser)  : argCropWidth.getInt(parser);
	opts.cropTop    = parser.isSet(argCropTop)    ? argCropTop.getInt(parser)    : argCropHeight.getInt(parser);
	opts.cropBottom = parser.isSet(argCropBottom) ? argCropBottom.getInt(parser) : argCropHeight.getInt(parser);

	opts.video3DSBS = parser.isSet(arg3DSBS);
	opts.video3DTAB = parser.isSet(arg3DTAB);

	opts.signalDetectionEnabled = !parser.isSet(argSignalDetection);
	opts.signalThresholdRed   = std::min(1.0, std::max(0.0, parser.isSet(argRedSignalThreshold)   ? argRedSignalThreshold.getDouble(parser)   : argSignalThreshold.getDouble(parser)));
	opts.signalThresholdGreen = std::min(1.0, std::max(0.0, parser.isSet(argGreenSignalThreshold) ? argGreenSignalThreshold.getDouble(parser) : argSignalThreshold.getDouble(parser)));
	opts.signalThresholdBlue  = std::min(1.0, std::max(0.0, parser.isSet(argBlueSignalThreshold)  ? argBlueSignalThreshold.getDouble(parser)  : argSignalThreshold.getDouble(parser)));

	bool signalAreaOptsOk = true;
	if (parser.isSet(argSignalHorizontalMin) != parser.isSet(argSignalVerticalMin))
	{
		signalAreaOptsOk = false;
	}

	if (parser.isSet(argSignalHorizontalMin) != parser.isSet(argSignalHorizontalMax))
	{
		signalAreaOptsOk = false;
	}

	if (parser.isSet(argSignalHorizontalMin) != parser.isSet(argSignalVerticalMax))
	{
		signalAreaOptsOk = false;
	}

	if (!signalAreaOptsOk)
	{
		opts.valid = false;
		opts.error = QStringLiteral("Wrong parameters given: --signal-[vertical|horizontal]-[min|max] options must be used together");
	}
	else
	{
		double const x_frac_min = argSignalHorizontalMin.getDouble(parser);
		double const y_frac_min = argSignalVerticalMin.getDouble(parser);
		double const x_frac_max = argSignalHorizontalMax.getDouble(parser);
		double const y_frac_max = argSignalVerticalMax.getDouble(parser);

		if (x_frac_min < 0.0 || y_frac_min < 0.0 || x_frac_max < 0.0 || y_frac_max < 0.0 ||
			x_frac_min > 1.0 || y_frac_min > 1.0 || x_frac_max > 1.0 || y_frac_max > 1.0)
		{
			opts.valid = false;
			opts.error = QStringLiteral("Wrong parameters given: --signal-[vertical|horizontal]-[min|max] values have to be between 0.0 and 1.0");
		}
		else if (parser.isSet(argSignalHorizontalMin))
		{
			opts.applySignalDetectionOffset = true;
			opts.signalHorizontalMin = x_frac_min;
			opts.signalVerticalMin   = y_frac_min;
			opts.signalHorizontalMax = x_frac_max;
			opts.signalVerticalMax   = y_frac_max;
		}
	}

	opts.address    = argAddress.value(parser);
	opts.priority   = argPriority.getInt(parser);
	opts.skipReply  = parser.isSet(argSkipReply);
	opts.screenshot = parser.isSet(argScreenshot);
	opts.debug      = parser.isSet(argDebug);
	opts.help       = parser.isSet(argHelp);

	qCDebug(grabber_flow).noquote()
		<< "Parsed V4L2 grabber options:"
		<< "\n  device:" << opts.device
		<< "input:" << opts.input
		<< "videoStandard:" << VideoStandard2String(opts.videoStandard)
		<< "pixelFormat:" << pixelFormatToString(opts.pixelFormat)
		<< "fps:" << opts.fps
		<< "\n  width:" << opts.width
		<< "height:" << opts.height
		<< "sizeDecimation:" << opts.sizeDecimation
		<< "flipMode:" << flipModeToString(opts.flipMode)
		<< "\n  cropLeft:" << opts.cropLeft
		<< "cropRight:" << opts.cropRight
		<< "cropTop:" << opts.cropTop
		<< "cropBottom:" << opts.cropBottom
		<< "\n  video3DSBS:" << opts.video3DSBS
		<< "video3DTAB:" << opts.video3DTAB
		<< "\n  signalDetectionEnabled:" << opts.signalDetectionEnabled
		<< "signalThresholdRed:" << opts.signalThresholdRed
		<< "signalThresholdGreen:" << opts.signalThresholdGreen
		<< "signalThresholdBlue:" << opts.signalThresholdBlue
		<< "\n  applySignalDetectionOffset:" << opts.applySignalDetectionOffset
		<< "signalHorizontalMin:" << opts.signalHorizontalMin
		<< "signalVerticalMin:" << opts.signalVerticalMin
		<< "signalHorizontalMax:" << opts.signalHorizontalMax
		<< "signalVerticalMax:" << opts.signalVerticalMax
		<< "\n  address:" << opts.address
		<< "priority:" << opts.priority
		<< "skipReply:" << opts.skipReply
		<< "screenshot:" << opts.screenshot
		<< "debug:" << opts.debug
		<< "\n  valid:" << opts.valid
		<< "error:" << opts.error;

	return opts;
}
