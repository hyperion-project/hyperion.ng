

// QT includes
#include <QCoreApplication>
#include <QTimer>
#include <QHostAddress>
#include <QImage>

#include <utils/DefaultSignalHandler.h>
#include <utils/ErrorManager.h>

#include "HyperionConfig.h"
#include <commandline/Parser.h>

#include "OsxWrapper.h"
#include <utils/ColorRgb.h>
#include <utils/Image.h>

#ifdef ENABLE_MDNS
// mDNS discover
#include <mdns/MdnsBrowser.h>
#else
// ssdp discover
#include <ssdp/SSDPDiscover.h>
#endif
#include <utils/NetUtils.h>

#include <flatbufserver/FlatBufferConnection.h>

using namespace commandline;

namespace {
inline const QString CAPTURE_TYPE = QStringLiteral("OsX-Grabber");
}
// save the image as screenshot
void saveScreenshot(const QString& filename, const Image<ColorRgb> & image)
{
	// store as PNG
	QImage const pngImage(reinterpret_cast<const uint8_t *>(image.memptr()), image.width(), image.height(), 3 * image.width(), QImage::Format_RGB888);
	pngImage.save(filename);
}

int main(int argc, char ** argv)
{
	DefaultSignalHandler::install();
	ErrorManager errorManager;

	QSharedPointer<Logger> log = Logger::getInstance(CAPTURE_TYPE.toUpper());
	Logger::setLogLevel(Logger::LogLevel::Info);
	
	QCoreApplication const app(argc, argv);

	QString const baseName = QCoreApplication::applicationName();
	std::cout << baseName.toStdString() << ":\n"
			  << "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ") - " << BUILD_TIMESTAMP << "\n";

	QObject::connect(&errorManager, &ErrorManager::errorOccurred, [&log](const QString &error) {
		Error(log, "Error occured: %s", QSTRING_CSTR(error));
		QTimer::singleShot(0, []() { QCoreApplication::quit(); }); 
	});

	// Force locale to have predictable, minimal behavior while still supporting full Unicode.
	setlocale(LC_ALL, "C.UTF-8");
	QLocale::setDefault(QLocale::c());

	// create the option parser and initialize all parameters
	Parser parser( CAPTURE_TYPE + " capture application for Hyperion. Will automatically search a Hyperion server if -a option is not used. Please note that if you have more than one server running it's more or less random which one will be used.");

	IntOption &argDisplay = parser.add<IntOption>('d', "display", "Set the display to capture [default: %1]", "0");

	IntOption &argFps = parser.add<IntOption>('f', "framerate", QString("Capture frame rate. Range %1-%2fps").arg(GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ).arg(GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ), QString::number(GrabberWrapper::DEFAULT_RATE_HZ), GrabberWrapper::DEFAULT_MIN_GRAB_RATE_HZ, GrabberWrapper::DEFAULT_MAX_GRAB_RATE_HZ);
	IntOption &argSizeDecimation = parser.add<IntOption>('s', "size-decimator", "Decimation factor for the output image size [default=%1]", QString::number(GrabberWrapper::DEFAULT_PIXELDECIMATION), 1);

	IntOption &argCropLeft = parser.add<IntOption>(0x0, "crop-left", "Number of pixels to crop from the left of the picture before decimation");
	IntOption &argCropRight = parser.add<IntOption>(0x0, "crop-right", "Number of pixels to crop from the right of the picture before decimation");
	IntOption &argCropTop = parser.add<IntOption>(0x0, "crop-top", "Number of pixels to crop from the top of the picture before decimation");
	IntOption &argCropBottom = parser.add<IntOption>(0x0, "crop-bottom", "Number of pixels to crop from the bottom of the picture before decimation");
	BooleanOption const &arg3DSBS = parser.add<BooleanOption>(0x0, "3DSBS", "Interpret the incoming video stream as 3D side-by-side");
	BooleanOption const &arg3DTAB = parser.add<BooleanOption>(0x0, "3DTAB", "Interpret the incoming video stream as 3D top-and-bottom");

	Option const &argAddress = parser.add<Option>('a', "address", "The hostname or IP-address (IPv4 or IPv6) of the hyperion server.\nDefault host: %1, port: 19400.\nSample addresses:\nHost : hyperion.fritz.box\nIPv4 : 127.0.0.1:19400\nIPv6 : [2001:1:2:3:4:5:6:7]", "127.0.0.1");
	IntOption &argPriority = parser.add<IntOption>('p', "priority", "Use the provided priority channel (suggested 100-199) [default: %1]", "150");
	BooleanOption const &argSkipReply = parser.add<BooleanOption>(0x0, "skip-reply", "Do not receive and check reply messages from Hyperion");

	BooleanOption const &argScreenshot = parser.add<BooleanOption>(0x0, "screenshot", "Take a single screenshot, save it to file and quit");

	BooleanOption const &argDebug = parser.add<BooleanOption>(0x0, "debug", "Enable debug logging");
	BooleanOption const &argHelp = parser.add<BooleanOption>('h', "help", "Show this help message and exit");

	// parse all arguments
	parser.process(app);

	// check if debug logging is required
	if (parser.isSet(argDebug))
	{
		Logger::setLogLevel(Logger::LogLevel::Debug);
	}

	// check if we need to display the usage. exit if we do.
	if (parser.isSet(argHelp))
	{
		parser.showHelp(0);
	}

	OsxWrapper grabber (
				argFps.getInt(parser),
				argDisplay.getInt(parser),
				argSizeDecimation.getInt(parser),
				argCropLeft.getInt(parser),
				argCropRight.getInt(parser),
				argCropTop.getInt(parser),
				argCropBottom.getInt(parser)
				);

	if (!grabber.screenInit())
	{
		emit errorManager.errorOccurred("Failed to initialise the screen/display for this grabber");
		return 1;
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

	if (parser.isSet(argScreenshot))
	{
		// Capture a single screenshot and finish
		const Image<ColorRgb> &screenshot = grabber.getScreenshot();
		QString const fileName = "screenshot.png";
		saveScreenshot(fileName, screenshot);
		Info(log, "Screenshot saved as: \"%s\"", QSTRING_CSTR(fileName));
	}
	else
	{
		QString hostName;
		int port{FLATBUFFER_DEFAULT_PORT};

		// Split hostname and port (or use default port)
		QString const givenAddress = argAddress.value(parser);

		if (!NetUtils::resolveHostPort(givenAddress, hostName, port))
		{
			emit errorManager.errorOccurred(QString("Wrong address: unable to parse address (%1)").arg(givenAddress));
			return 1;
		}

		Info(log, "Connecting to Hyperion host: %s, port: %u", QSTRING_CSTR(hostName), port);

		if (MdnsBrowser::isMdns(hostName))
		{
			NetUtils::discoverMdnsServices("flatbuffer");
		}

		if (!NetUtils::convertMdnsToIp(log, hostName, port))
		{
			emit errorManager.errorOccurred(QString("IP-address cannot be resolved for the given mDNS service- or hostname: \"%1\"").arg(QSTRING_CSTR(hostName)));
			return 1;
		}

		// Create the FlatBuffer-connection
		FlatBufferConnection const flatbuf(CAPTURE_TYPE + " Standalone",
										   hostName,
										   argPriority.getInt(parser),
										   parser.isSet(argSkipReply), static_cast<quint16>(port));

		// Connect the screen capturing to flatbuf connection processing
		QObject::connect(&grabber, &OsxWrapper::sig_screenshot,
						 &flatbuf,
						 static_cast<void (FlatBufferConnection::*)(const Image<ColorRgb>&)>(&FlatBufferConnection::setImage));


		QObject::connect(&flatbuf, &FlatBufferConnection::isReadyToSend, [&log, &grabber]() {
			Debug(log,"Start grabber");
			grabber.start(); 
		});

		QObject::connect(&flatbuf, &FlatBufferConnection::isDisconnected, [&log, &grabber]() {
			Debug(log,"Stop grabber");
			grabber.stop(); 
		});

		QObject::connect(&flatbuf, &FlatBufferConnection::errorOccured, [&log, &grabber, &errorManager](const QString &error) {
			Debug(log,"Stop grabber");
			grabber.stop();
			emit errorManager.errorOccurred(error); 
		});

		// Start the application
		QCoreApplication::exec();
	}
	
	Logger::deleteInstance();
	
	return 0;
}
