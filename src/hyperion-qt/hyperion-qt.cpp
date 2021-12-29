
// QT includes
#include <QCoreApplication>
#include <QImage>

#include "QtWrapper.h"
#include <utils/ColorRgb.h>
#include <utils/Image.h>

#include "HyperionConfig.h"
#include <commandline/Parser.h>

// mDNS discover
#include <qmdnsengine/server.h>
#include <qmdnsengine/service.h>
#include <qmdnsengine/browser.h>
#include <qmdnsengine/resolver.h>
#include <qmdnsengine/cache.h>
#include <utils/NetUtils.h>

//flatbuf sending
#include <flatbufserver/FlatBufferConnection.h>

// Constants
namespace {

const char HYPERION_MDNS_SERVICE_TYPE[] = "_hyperiond-flatbuf._tcp.local.";
const char HYPERION_SERVICENAME[] = "hyperion-flatbuffer";

constexpr std::chrono::milliseconds DEFAULT_DISCOVER_TIMEOUT{ 2000 };
constexpr std::chrono::milliseconds DEFAULT_ADDRESS_RESOLVE_TIMEOUT{ 2000 };

} //End of constants

using namespace commandline;

// save the image as screenshot
void saveScreenshot(const QString& filename, const Image<ColorRgb> & image)
{
	// store as PNG
	QImage pngImage(reinterpret_cast<const uint8_t *>(image.memptr()), static_cast<int>(image.width()), static_cast<int>(image.height()), static_cast<int>(3*image.width()), QImage::Format_RGB888);
	pngImage.save(filename);
}

int main(int argc, char ** argv)
{
	Logger *log = Logger::getInstance("QTGRABBER");
	Logger::setLogLevel(Logger::INFO);

	std::cout
		<< "hyperion-qt:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	//QCoreApplication app(argc, argv);
	QGuiApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("Qt interface capture application for Hyperion. Will automatically search a Hyperion server if -a option isn't used. Please note that if you have more than one server running it's more or less random which one will be used.");

		IntOption      & argDisplay         = parser.add<IntOption>    ('d', "display",        "Set the display to capture [default: %1]", "0");

		IntOption      & argFps				= parser.add<IntOption>    ('f', "framerate",      QString("Capture frame rate. Range %1-%2fps").arg(GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ).arg(GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ),  QString::number(GrabberWrapper::DEFAULT_RATE_HZ), GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ, GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ);
		IntOption      & argSizeDecimation	= parser.add<IntOption>    ('s', "size-decimator", "Decimation factor for the output image size [default=%1]", QString::number(GrabberWrapper::DEFAULT_PIXELDECIMATION), 1);

		IntOption      & argCropLeft        = parser.add<IntOption>    (0x0, "crop-left",      "Number of pixels to crop from the left of the picture before decimation");
		IntOption      & argCropRight       = parser.add<IntOption>    (0x0, "crop-right",     "Number of pixels to crop from the right of the picture before decimation");
		IntOption      & argCropTop         = parser.add<IntOption>    (0x0, "crop-top",       "Number of pixels to crop from the top of the picture before decimation");
		IntOption      & argCropBottom      = parser.add<IntOption>    (0x0, "crop-bottom",    "Number of pixels to crop from the bottom of the picture before decimation");
		BooleanOption  & arg3DSBS			= parser.add<BooleanOption>(0x0, "3DSBS",          "Interpret the incoming video stream as 3D side-by-side");
		BooleanOption  & arg3DTAB			= parser.add<BooleanOption>(0x0, "3DTAB",          "Interpret the incoming video stream as 3D top-and-bottom");

		Option         & argAddress         = parser.add<Option>       ('a', "address",        "The hostname or IP-address (IPv4 or IPv6) of the hyperion server.\nDefault host: %1, port: 19400.\nSample addresses:\nHost : hyperion.fritz.box\nIPv4 : 127.0.0.1:19400\nIPv6 : [2001:1:2:3:4:5:6:7]", "127.0.0.1");
		IntOption      & argPriority        = parser.add<IntOption>    ('p', "priority",       "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
		BooleanOption  & argSkipReply       = parser.add<BooleanOption>(0x0, "skip-reply",     "Do not receive and check reply messages from Hyperion");

		BooleanOption  & argScreenshot      = parser.add<BooleanOption>(0x0, "screenshot",     "Take a single screenshot, save it to file and quit");

		BooleanOption  & argDebug           = parser.add<BooleanOption>(0x0, "debug",          "Enable debug logging");
		BooleanOption  & argHelp            = parser.add<BooleanOption>('h', "help",           "Show this help message and exit");

		// parse all arguments
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

		QtWrapper qtWrapper(
			argFps.getInt(parser),
			argDisplay.getInt(parser),
			argSizeDecimation.getInt(parser),
			argCropLeft.getInt(parser),
			argCropRight.getInt(parser),
			argCropTop.getInt(parser),
			argCropBottom.getInt(parser)
			);

		if (!qtWrapper.displayInit())
		{
			Error(log, "Failed to initialise the screen/display for this grabber");
			return -1;
		}

		// set 3D mode if applicable
		if (parser.isSet(arg3DSBS))
		{
			qtWrapper.setVideoMode(VideoMode::VIDEO_3DSBS);
		}
		else if (parser.isSet(arg3DTAB))
		{
			qtWrapper.setVideoMode(VideoMode::VIDEO_3DTAB);
		}

		if (parser.isSet(argScreenshot))
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> &screenshot = qtWrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			QString hostName;
			QString serviceName {HYPERION_SERVICENAME};
			quint16 port {FLATBUFFER_DEFAULT_PORT};

			// Split hostname and port (or use default port)
			QString address = argAddress.value(parser);
			if (!NetUtils::resolveHostPort(address, hostName, port))
			{
				throw std::runtime_error(QString("Wrong address: unable to parse address (%1)").arg(address).toStdString());
			}

			QMdnsEngine::Server server;
			QMdnsEngine::Cache cache;

			// Search available Hyperion services via mDNS, if default/localhost IP is given
			if(hostName == "127.0.0.1" || hostName == "::1")
			{
				QMdnsEngine::Service firstMdnsService;

				QMdnsEngine::Browser browser(&server, HYPERION_MDNS_SERVICE_TYPE, &cache);
				QObject::connect(&browser, &QMdnsEngine::Browser::serviceAdded, log,
								  [&firstMdnsService, &log](const QMdnsEngine::Service &service) {
									  Debug(log, "Discovered Hyperion service [%s] at host: %s, port: %u", service.name().constData(), service.hostname().constData(), service.port());

									  // Use the first service discovered
									  if (firstMdnsService.name().isEmpty())
									  {
										  firstMdnsService = service;
									  }
								  });

				QEventLoop loop;
				QTimer t;
				QObject::connect(&browser, &QMdnsEngine::Browser::serviceAdded, &loop, &QEventLoop::quit);
				QTimer::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
				t.start(DEFAULT_DISCOVER_TIMEOUT.count());
				loop.exec();

				if (firstMdnsService.hostname().isEmpty())
				{
					throw std::runtime_error(QString("Automatic discovery failed! No Hyperion servers found providing a service of type: %1")
												  .arg(QString(HYPERION_MDNS_SERVICE_TYPE)).toStdString()
											  );

				}
				serviceName = firstMdnsService.name();
				hostName = firstMdnsService.hostname();
				port = firstMdnsService.port();
			}

			//Resolve Hostname to HostAddress
			QString hostAddress;

			if (hostName.endsWith(".local"))
			{
				hostName.append('.');
			}

			if (hostName.endsWith(".local."))
			{
				QMdnsEngine::Resolver resolver(&server, hostName.toUtf8(), &cache);
				QObject::connect(&resolver, &QMdnsEngine::Resolver::resolved, log,
								  [hostName, &log, &hostAddress](const QHostAddress &resolvedAddress) {
									  Debug(log, "Resolved Hyperion hostname: %s to address: %s",  QSTRING_CSTR(hostName), QSTRING_CSTR(resolvedAddress.toString()));

									  // Use the first resolved address
									  if (hostAddress.isEmpty())
									  {
										  hostAddress = resolvedAddress.toString();
									  }
								  }
								  );

				QEventLoop loop;
				QTimer t;
				QObject::connect(&resolver, &QMdnsEngine::Resolver::resolved, &loop, &QEventLoop::quit);
				QTimer::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
				t.start(DEFAULT_ADDRESS_RESOLVE_TIMEOUT.count());
				loop.exec();

				if (hostAddress.isEmpty() )
				{
					throw std::runtime_error(QString("Address could not be resolved for hostname: %2").arg(QSTRING_CSTR(hostName)).toStdString());
				}

			}
			else
			{
				hostAddress = hostName;
			}

			Info(log, "Connecting to Hyperion host: %s, port: %u using service: %s", QSTRING_CSTR(hostName), port, QSTRING_CSTR(serviceName));


			// Create the Flabuf-connection
			FlatBufferConnection flatbuf("Qt Standalone", hostAddress, argPriority.getInt(parser), parser.isSet(argSkipReply), port);

			// Connect the screen capturing to flatbuf connection processing
			QObject::connect(&qtWrapper, &QtWrapper::sig_screenshot, &flatbuf, &FlatBufferConnection::setImage);

			// Start the capturing
			qtWrapper.start();

			// Start the application
			QGuiApplication::exec();
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
