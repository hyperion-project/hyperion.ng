#include "QtGrabberTraits.h"

#include <QCoreApplication>

#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#else
#include <ssdp/SSDPDiscover.h>
#endif

#include <flatbufserver/FlatBufferConnection.h>
#include <utils/Logger.h>
#include <utils/NetUtils.h>

#include "QtGrabberCli.h"
#include "QtWrapper.h"
#include "ScreenshotUtil.h"

void QtGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

QtGrabberOptions QtGrabberTraits::parseOptions(QCoreApplication& app)
{
	return parseQtGrabberOptions(app);
}

int QtGrabberTraits::run(QCoreApplication& /*app*/,
                         const QtGrabberOptions& opts,
                         QSharedPointer<Logger> log,
                         ErrorManager& errorManager)
{
	QtWrapper grabber(
		opts.fps,
		opts.display,
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
		QString const fileName = QStringLiteral("screenshot.png");
		saveScreenshot(fileName, screenshot);
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
		QStringLiteral("Qt-Grabber Standalone"),
		hostName,
		opts.priority,
		opts.skipReply,
		static_cast<quint16>(port));

	// Connect the screen capturing to flatbuf connection processing
	QObject::connect(&grabber, &QtWrapper::sig_screenshot,
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
