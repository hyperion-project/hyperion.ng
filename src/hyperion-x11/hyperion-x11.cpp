
// QT includes
#include <QApplication>
#include <QImage>

#include <commandline/Parser.h>
#include <flatbufserver/FlatBufferConnection.h>
#include <utils/DefaultSignalHandler.h>
#include "X11Wrapper.h"
#include "HyperionConfig.h"

#ifdef ENABLE_MDNS
// mDNS discover
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#else
// ssdp discover
#include <ssdp/SSDPDiscover.h>
#endif
#include <utils/NetUtils.h>

using namespace commandline;

// save the image as screenshot
void saveScreenshot(QString filename, const Image<ColorRgb> & image)
{
	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save(filename);
}

int main(int argc, char ** argv)
{
	Logger *log = Logger::getInstance("X11GRABBER");
	Logger::setLogLevel(Logger::INFO);

	std::cout
		<< "hyperion-x11:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	DefaultSignalHandler::install();

	QApplication app(argc, argv);
	try
	{
		// create the option parser and initialize all parameters
		Parser parser("X11 capture application for Hyperion. Will automatically search a Hyperion server if -a option isn't used. Please note that if you have more than one server running it's more or less random which one will be used.");

		IntOption           & argFps             = parser.add<IntOption>    ('f', "framerate",      QString("Capture frame rate. Range %1-%2fps").arg(GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ).arg(GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ), QString::number(GrabberWrapper::DEFAULT_RATE_HZ), GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ, GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ);
		IntOption           & argSizeDecimation	 = parser.add<IntOption>    ('s', "size-decimator", "Decimation factor for the output image size [default=%1]", QString::number(GrabberWrapper::DEFAULT_PIXELDECIMATION), 1);

		IntOption           & argCropWidth       = parser.add<IntOption>    (0x0, "crop-width",     "Number of pixels to crop from the left and right sides of the picture before decimation [default: %1]", "0");
		IntOption           & argCropHeight      = parser.add<IntOption>    (0x0, "crop-height",    "Number of pixels to crop from the top and the bottom of the picture before decimation [default: %1]", "0");
		IntOption           & argCropLeft        = parser.add<IntOption>    (0x0, "crop-left",      "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
		IntOption           & argCropRight       = parser.add<IntOption>    (0x0, "crop-right",     "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
		IntOption           & argCropTop         = parser.add<IntOption>    (0x0, "crop-top",       "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
		IntOption           & argCropBottom      = parser.add<IntOption>    (0x0, "crop-bottom",    "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
		BooleanOption       & arg3DSBS			 = parser.add<BooleanOption>(0x0, "3DSBS",          "Interpret the incoming video stream as 3D side-by-side");
		BooleanOption       & arg3DTAB			 = parser.add<BooleanOption>(0x0, "3DTAB",          "Interpret the incoming video stream as 3D top-and-bottom");

		Option              & argAddress         = parser.add<Option>       ('a', "address",        "The hostname or IP-address (IPv4 or IPv6) of the hyperion server.\nDefault host: %1, port: 19400.\nSample addresses:\nHost : hyperion.fritz.box\nIPv4 : 127.0.0.1:19400\nIPv6 : [2001:1:2:3:4:5:6:7]", "127.0.0.1");
		IntOption           & argPriority        = parser.add<IntOption>    ('p', "priority",       "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
		BooleanOption       & argSkipReply       = parser.add<BooleanOption>(0x0, "skip-reply",     "Do not receive and check reply messages from Hyperion");

		BooleanOption       & argScreenshot      = parser.add<BooleanOption>(0x0, "screenshot",     "Take a single screenshot, save it to file and quit");

		BooleanOption       & argDebug           = parser.add<BooleanOption>(0x0, "debug",          "Enable debug logging");
		BooleanOption       & argHelp            = parser.add<BooleanOption>('h', "help",           "Show this help message and exit");

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

		// Create the X11 grabbing stuff
		X11Wrapper x11Wrapper(
			argFps.getInt(parser),
			argSizeDecimation.getInt(parser),
			parser.isSet(argCropLeft) ? argCropLeft.getInt(parser) : argCropWidth.getInt(parser),
			parser.isSet(argCropRight) ? argCropRight.getInt(parser) : argCropWidth.getInt(parser),
			parser.isSet(argCropTop) ? argCropTop.getInt(parser) : argCropHeight.getInt(parser),
			parser.isSet(argCropBottom) ? argCropBottom.getInt(parser) : argCropHeight.getInt(parser)
			);

		if (!x11Wrapper.displayInit())
		{
			Error(log, "Failed to initialise the screen/display for this grabber");
			return -1;
		}

		// set 3D mode if applicable
		if (parser.isSet(arg3DSBS))
		{
			x11Wrapper.setVideoMode(VideoMode::VIDEO_3DSBS);
		}
		else if (parser.isSet(arg3DTAB))
		{
			x11Wrapper.setVideoMode(VideoMode::VIDEO_3DTAB);
		}

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> & screenshot = x11Wrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
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
			FlatBufferConnection flatbuf("X11 Standalone", host, argPriority.getInt(parser), parser.isSet(argSkipReply), port);

			// Connect the screen capturing to flatbuf connection processing
			QObject::connect(&x11Wrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &flatbuf, SLOT(setImage(Image<ColorRgb>)));

			// Start the capturing
			x11Wrapper.start();

			// Start the application
			app.exec();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occurred. Display error and quit
		Error(log, "%s", e.what());
		return -1;
	}

	return 0;
}
