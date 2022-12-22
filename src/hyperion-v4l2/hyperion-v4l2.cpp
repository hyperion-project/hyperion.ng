// STL includes
#include <csignal>
#include <iomanip>
#include <clocale>

// QT includes
#include <QCoreApplication>

// blackborder includes
#include <blackborder/BlackBorderProcessor.h>

// grabber includes
#include "grabber/V4L2Grabber.h"

// flatbuf includes
#include <flatbufserver/FlatBufferConnection.h>

// hyperion-v4l2 includes
#include "ScreenshotHandler.h"

#include "HyperionConfig.h"
#include <commandline/Parser.h>

#ifdef ENABLE_MDNS
// mDNS discover
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#else
// ssdp discover
#include <ssdp/SSDPDiscover.h>
#endif
#include <utils/NetUtils.h>

#include <utils/DefaultSignalHandler.h>

using namespace commandline;

int main(int argc, char** argv)
{
	Logger *log = Logger::getInstance("V4L2GRABBER");
	Logger::setLogLevel(Logger::INFO);

	std::cout
		<< "hyperion-v4l2:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	QCoreApplication app(argc, argv);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	// register the image type to use in signals
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("V4L capture application for Hyperion.  Will automatically search a Hyperion server if -a option isn't used. Please note that if you have more than one server running it's more or less random which one will be used.");

		Option             & argDevice              = parser.add<Option>       ('d', "device", "The device to use, can be /dev/video0 [default: %1 (auto detected)]", "auto");
		IntOption          & argInput               = parser.add<IntOption>    ('i', "input",  "The device input [default: %1]", "0");
		SwitchOption<VideoStandard> & argVideoStandard= parser.add<SwitchOption<VideoStandard>>('v', "video-standard", "The used video standard. Valid values are PAL, NTSC, SECAM or no-change. [default: %1]", "no-change");
		SwitchOption<PixelFormat> & argPixelFormat   = parser.add<SwitchOption<PixelFormat>>  (0x0, "pixel-format", "The use pixel format. Valid values are YUYV, UYVY, RGB32, MJPEG or no-change. [default: %1]", "no-change");
		IntOption          & argFps                 = parser.add<IntOption>    ('f', "framerate", QString("Capture frame rate. Range %1-%2fps").arg(GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ).arg(GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ), QString::number(GrabberWrapper::DEFAULT_RATE_HZ), GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ, GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ);

		SwitchOption<FlipMode> & argFlipMode        = parser.add<SwitchOption<FlipMode>>(0x0, "flip-mode", "The used image flip mode. Valid values are HORIZONTAL, VERTICAL, BOTH or no-change. [default: %1]", "no-change");
		IntOption          & argWidth               = parser.add<IntOption>    ('w', "width", "Width of the captured image [default: %1]", "640", 640);
		IntOption          & argHeight              = parser.add<IntOption>    ('h', "height", "Height of the captured image [default: %1]", "480", 480);
		IntOption          & argSizeDecimation	    = parser.add<IntOption>    ('s', "size-decimator", "Decimation factor for the output image size [default=%1]", QString::number(GrabberWrapper::DEFAULT_PIXELDECIMATION), 1);
		IntOption          & argCropWidth           = parser.add<IntOption>    (0x0, "crop-width", "Number of pixels to crop from the left and right sides of the picture before decimation [default: %1]", "0");
		IntOption          & argCropHeight          = parser.add<IntOption>    (0x0, "crop-height", "Number of pixels to crop from the top and the bottom of the picture before decimation [default: %1]", "0");
		IntOption          & argCropLeft            = parser.add<IntOption>    (0x0, "crop-left", "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
		IntOption          & argCropRight           = parser.add<IntOption>    (0x0, "crop-right", "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
		IntOption          & argCropTop             = parser.add<IntOption>    (0x0, "crop-top", "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
		IntOption          & argCropBottom          = parser.add<IntOption>    (0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
		BooleanOption      & arg3DSBS               = parser.add<BooleanOption>(0x0, "3DSBS", "Interpret the incoming video stream as 3D side-by-side");
		BooleanOption      & arg3DTAB               = parser.add<BooleanOption>(0x0, "3DTAB", "Interpret the incoming video stream as 3D top-and-bottom");


		BooleanOption      & argSignalDetection     = parser.add<BooleanOption>(0x0, "signal-detection-disabled", "disable signal detection");
		DoubleOption       & argSignalThreshold     = parser.add<DoubleOption> (0x0, "signal-threshold", "The signal threshold for detecting the presence of a signal. Value should be between 0.0 and 1.0.", QString(), 0.0, 1.0);
		DoubleOption       & argRedSignalThreshold  = parser.add<DoubleOption> (0x0, "red-threshold", "The red signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)");
		DoubleOption       & argGreenSignalThreshold= parser.add<DoubleOption> (0x0, "green-threshold", "The green signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)");
		DoubleOption       & argBlueSignalThreshold = parser.add<DoubleOption> (0x0, "blue-threshold", "The blue signal threshold. Value should be between 0.0 and 1.0. (overrides --signal-threshold)");
		DoubleOption       & argSignalHorizontalMin = parser.add<DoubleOption> (0x0, "signal-horizontal-min", "area for signal detection - horizontal minimum offset value. Values between 0.0 and 1.0");
		DoubleOption       & argSignalVerticalMin   = parser.add<DoubleOption> (0x0, "signal-vertical-min"  , "area for signal detection - vertical minimum offset value. Values between 0.0 and 1.0");
		DoubleOption       & argSignalHorizontalMax = parser.add<DoubleOption> (0x0, "signal-horizontal-max", "area for signal detection - horizontal maximum offset value. Values between 0.0 and 1.0");
		DoubleOption       & argSignalVerticalMax   = parser.add<DoubleOption> (0x0, "signal-vertical-max"  , "area for signal detection - vertical maximum offset value. Values between 0.0 and 1.0");

		Option             & argAddress             = parser.add<Option>       ('a', "address", "The hostname or IP-address (IPv4 or IPv6) of the hyperion server.\nDefault host: %1, port: 19400.\nSample addresses:\nHost : hyperion.fritz.box\nIPv4 : 127.0.0.1:19400\nIPv6 : [2001:1:2:3:4:5:6:7]", "127.0.0.1");
		IntOption          & argPriority            = parser.add<IntOption>    ('p', "priority", "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
		BooleanOption      & argSkipReply           = parser.add<BooleanOption>(0x0, "skip-reply", "Do not receive and check reply messages from Hyperion");

		BooleanOption      & argScreenshot          = parser.add<BooleanOption>('S', "screenshot", "Take a single screenshot, save it to file and quit");

		BooleanOption      & argDebug               = parser.add<BooleanOption>(0x0, "debug", "Enable debug logging");
		BooleanOption      & argHelp                = parser.add<BooleanOption>(0x0, "help", "Show this help message and exit");

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

		// check if debug logging is required
		if (parser.isSet(argDebug))
		{
			Logger::setLogLevel(Logger::DEBUG);
		}

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			parser.showHelp(0);
		}

		// initialize the grabber
		V4L2Grabber grabber;

		// set device
		grabber.setDevice(argDevice.value(parser), "");

		// set input
		grabber.setInput(argInput.getInt(parser));

		// set resolution
		grabber.setWidthHeight(argWidth.getInt(parser), argHeight.getInt(parser));

		// set fps
		if (parser.isSet(argFps))
			grabber.setFramerate(argFps.getInt(parser));

		// set encoding format
		if (parser.isSet(argPixelFormat))
			grabber.setEncoding(pixelFormatToString(argPixelFormat.switchValue(parser)));

		// set video standard
		if (parser.isSet(argVideoStandard))
			grabber.setVideoStandard(argVideoStandard.switchValue(parser));

		// set image size decimation
		grabber.setPixelDecimation(std::max(1, argSizeDecimation.getInt(parser)));

		// set flip mode
		grabber.setFlipMode(argFlipMode.switchValue(parser));

		// set signal detection
		grabber.setSignalDetectionEnable(! parser.isSet(argSignalDetection));
		grabber.setSignalThreshold(
					std::min(1.0, std::max(0.0, parser.isSet(argRedSignalThreshold)   ? argRedSignalThreshold.getDouble(parser)   : argSignalThreshold.getDouble(parser))),
					std::min(1.0, std::max(0.0, parser.isSet(argGreenSignalThreshold) ? argGreenSignalThreshold.getDouble(parser) : argSignalThreshold.getDouble(parser))),
					std::min(1.0, std::max(0.0, parser.isSet(argBlueSignalThreshold)  ? argBlueSignalThreshold.getDouble(parser)  : argSignalThreshold.getDouble(parser))),
					50);

		// set cropping values
		grabber.setCropping(
			parser.isSet(argCropLeft)   ? argCropLeft.getInt(parser)   : argCropWidth.getInt(parser),
			parser.isSet(argCropRight)  ? argCropRight.getInt(parser)  : argCropWidth.getInt(parser),
			parser.isSet(argCropTop)    ? argCropTop.getInt(parser)    : argCropHeight.getInt(parser),
			parser.isSet(argCropBottom) ? argCropBottom.getInt(parser) : argCropHeight.getInt(parser));

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
			Error(log, "aborting, because --signal-[vertical|horizontal]-[min|max] options must be used together");
			return 1;
		}

		double x_frac_min = argSignalHorizontalMin.getDouble(parser);
		double y_frac_min = argSignalVerticalMin.getDouble(parser);
		double x_frac_max = argSignalHorizontalMax.getDouble(parser);
		double y_frac_max = argSignalVerticalMax.getDouble(parser);

		if (x_frac_min<0.0 || y_frac_min<0.0 || x_frac_max<0.0 || y_frac_max<0.0 || x_frac_min>1.0 || y_frac_min>1.0 ||  x_frac_max>1.0 ||  y_frac_max>1.0)
		{
			Error(log, "aborting, because --signal-[vertical|horizontal]-[min|max] values have to be between 0.0 and 1.0");
			return 1;
		}

		if (parser.isSet(argSignalHorizontalMin))
		{
			grabber.setSignalDetectionOffset( x_frac_min, y_frac_min, x_frac_max, y_frac_max);
		}

		// set 3D mode if applicable
		if (parser.isSet(arg3DSBS))
		{
			grabber.setVideoMode(VideoMode::VIDEO_3DSBS);
		}
		else if (parser.isSet(arg3DTAB))
		{
			grabber.setVideoMode(VideoMode::VIDEO_3DTAB);
		}

		// run the grabber
		if (parser.isSet(argScreenshot))
		{
			const QRectF signalDetectionOffset = grabber.getSignalDetectionOffset();

			ScreenshotHandler handler("screenshot.png", signalDetectionOffset);
			QObject::connect(&grabber, SIGNAL(newFrame(Image<ColorRgb>)), &handler, SLOT(receiveImage(Image<ColorRgb>)));
			grabber.start();
			QCoreApplication::exec();
			grabber.stop();
		}
		else
		{
			QString host;
			QString serviceName{ QHostInfo::localHostName() };
			int port{ FLATBUFFER_DEFAULT_PORT };

			// Split hostname and port (or use default port)
			QString givenAddress = argAddress.value(parser);
			if (!NetUtils::resolveHostPort(givenAddress, host, port))
			{
				throw std::runtime_error(QString("Wrong address: unable to parse address (%1)").arg(givenAddress).toStdString());
			}

			// Search available Hyperion services via mDNS, if default/localhost IP is given
			if (host == "127.0.0.1" || host == "::1")
			{
#ifndef ENABLE_MDNS
				SSDPDiscover discover;
				host = discover.getFirstService(searchType::STY_FLATBUFSERVER);
#endif
				
				QHostAddress address;
				if (!NetUtils::resolveHostToAddress(log, host, address, port))
				{
					throw std::runtime_error(QString("Address could not be resolved for hostname: %2").arg(QSTRING_CSTR(host)).toStdString());
				}
				host = address.toString();
			}

			Info(log, "Connecting to Hyperion host: %s, port: %u using service: %s", QSTRING_CSTR(host), port, QSTRING_CSTR(serviceName));

			// Create the Flabuf-connection
			FlatBufferConnection flatbuf("V4L2 Standalone", host, argPriority.getInt(parser), parser.isSet(argSkipReply), port);

			// Connect the screen capturing to flatbuf connection processing
			QObject::connect(&grabber, SIGNAL(newFrame(const Image<ColorRgb> &)), &flatbuf, SLOT(setImage(Image<ColorRgb>)));

			// Start the capturing
			if (grabber.prepare())
				grabber.start();

			// Start the application
			app.exec();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		Error(log, "%s", e.what());
		return 1;
	}

	return 0;
}
