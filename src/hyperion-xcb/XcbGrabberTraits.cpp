#include "XcbGrabberTraits.h"

#include <QCoreApplication>
#include <QImage>

#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#else
#include <ssdp/SSDPDiscover.h>
#endif

#include <commandline/BooleanOption.h>
#include <commandline/IntOption.h>
#include <commandline/Option.h>
#include <commandline/Parser.h>
#include <flatbufserver/FlatBufferConnection.h>
#include <hyperion/GrabberWrapper.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>
#include <utils/NetUtils.h>

#include "XcbWrapper.h"

using namespace commandline;

void XcbGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

XcbGrabberOptions XcbGrabberTraits::parseOptions(const QCoreApplication& app)
{
	Parser parser(QStringLiteral("XCB-Grabber capture application for Hyperion. Will automatically search a Hyperion server if -a option is not used. Please note that if you have more than one server running it's more or less random which one will be used."));

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

	IntOption& argCropWidth  = parser.add<IntOption>(0x0, "crop-width",  "Number of pixels to crop from the left and right sides of the picture before decimation [default: %1]", "0");
	IntOption& argCropHeight = parser.add<IntOption>(0x0, "crop-height", "Number of pixels to crop from the top and the bottom of the picture before decimation [default: %1]", "0");
	IntOption& argCropLeft   = parser.add<IntOption>(0x0, "crop-left",   "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
	IntOption& argCropRight  = parser.add<IntOption>(0x0, "crop-right",  "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
	IntOption& argCropTop    = parser.add<IntOption>(0x0, "crop-top",    "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
	IntOption& argCropBottom = parser.add<IntOption>(0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
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

	XcbGrabberOptions opts;
	opts.fps            = argFps.getInt(parser);
	opts.sizeDecimation = argSizeDecimation.getInt(parser);
	opts.cropLeft       = parser.isSet(argCropLeft)   ? argCropLeft.getInt(parser)   : argCropWidth.getInt(parser);
	opts.cropRight      = parser.isSet(argCropRight)  ? argCropRight.getInt(parser)  : argCropWidth.getInt(parser);
	opts.cropTop        = parser.isSet(argCropTop)    ? argCropTop.getInt(parser)    : argCropHeight.getInt(parser);
	opts.cropBottom     = parser.isSet(argCropBottom) ? argCropBottom.getInt(parser) : argCropHeight.getInt(parser);
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

int XcbGrabberTraits::run(QCoreApplication& /*app*/,
                          const XcbGrabberOptions& opts,
                          QSharedPointer<Logger> log,
                          ErrorManager& errorManager)
{
	// Create the XCB grabbing stuff
	XcbWrapper grabber(
		opts.fps,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	if (!grabber.screenInit())
	{
		emit errorManager.errorOccurred(QStringLiteral("Failed to initialise the screen/display for this grabber"));
		return 1;
	}

	// set 3D mode if applicable
	if (opts.video3DSBS)
	{
		grabber.setVideoMode(VideoMode::VIDEO_3DSBS);
	}
	else if (opts.video3DTAB)
	{
		grabber.setVideoMode(VideoMode::VIDEO_3DTAB);
	}

	if (opts.screenshot)
	{
		// Capture a single screenshot and finish
		const Image<ColorRgb>& screenshot = grabber.getScreenshot();
		auto const fileName = QStringLiteral("screenshot.png");
		QImage const pngImage(
			reinterpret_cast<const uint8_t*>(screenshot.memptr()),
			screenshot.width(),
			screenshot.height(),
			3 * screenshot.width(),
			QImage::Format_RGB888);
		pngImage.save(fileName);
		Info(log, "Screenshot saved as: \"%s\"", QSTRING_CSTR(fileName));
		return 0;
	}

	QString hostName;
	int port{FLATBUFFER_DEFAULT_PORT};

	// Split hostname and port (or use default port)
	QString const givenAddress = opts.address;

	if (!NetUtils::resolveHostPort(givenAddress, hostName, port))
	{
		emit errorManager.errorOccurred(QString("Wrong address: unable to parse address (%1)").arg(givenAddress));
		return 1;
	}

	Info(log, "Connecting to Hyperion host: %s, port: %u", QSTRING_CSTR(hostName), port);

#ifdef ENABLE_MDNS
	if (MdnsBrowser::isMdns(hostName))
	{
		NetUtils::discoverMdnsServices("flatbuffer");
	}
#endif

	if (!NetUtils::convertMdnsToIp(log, hostName, port))
	{
		emit errorManager.errorOccurred(QString("IP-address cannot be resolved for the given mDNS service- or hostname: \"%1\"").arg(QSTRING_CSTR(hostName)));
		return 1;
	}

	// Create the FlatBuffer-connection
	FlatBufferConnection const flatbuf(
		QStringLiteral("XCB-Grabber Standalone"),
		hostName,
		opts.priority,
		opts.skipReply,
		static_cast<quint16>(port));

	// Connect the screen capturing to flatbuf connection processing
	QObject::connect(&grabber, &XcbWrapper::sig_screenshot,
	                 &flatbuf,
	                 static_cast<void (FlatBufferConnection::*)(const Image<ColorRgb>&)>(&FlatBufferConnection::setImage));

	QObject::connect(&flatbuf, &FlatBufferConnection::isReadyToSend, [&log, &grabber]() {
		Debug(log, "Start grabber");
		grabber.start();
	});

	QObject::connect(&flatbuf, &FlatBufferConnection::isDisconnected, [&log, &grabber]() {
		Debug(log, "Stop grabber");
		grabber.stop();
	});

	QObject::connect(&flatbuf, &FlatBufferConnection::errorOccured, [&log, &grabber, &errorManager](const QString& error) {
		Debug(log, "Stop grabber");
		grabber.stop();
		emit errorManager.errorOccurred(error);
	});

	// Start the application
	return QCoreApplication::exec();
}
