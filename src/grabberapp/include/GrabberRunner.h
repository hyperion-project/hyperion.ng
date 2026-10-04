#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QSharedPointer>
#include <QString>

#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#else
#include <ssdp/SSDPDiscover.h>
#endif

#include <flatbufserver/FlatBufferConnection.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>
#include <utils/NetUtils.h>
#include <utils/VideoMode.h>

#include "ScreenshotUtil.h"

///
/// Generic run-loop for standalone screen grabbers that stream frames to a Hyperion
/// server via a Flatbuffer connection (e.g. Qt-, X11- and XCB-Grabber).
///
/// The grabber itself is templated (@p WrapperType) so that each standalone grabber can
/// keep its own construction logic (which may need grabber-specific constructor
/// arguments, e.g. a display index) while sharing the otherwise identical wiring:
/// screen initialisation, 3D mode handling, screenshot mode, server address resolution
/// and the Flatbuffer connection/signal wiring.
///
/// @param grabberName   Human readable grabber name, used for logging and as the
///                      Flatbuffer connection origin (e.g. "Qt-Grabber").
/// @param grabber       An already constructed grabber wrapper. Must provide
///                      screenInit(), setVideoMode(VideoMode), getScreenshot(),
///                      start(), stop() and the sig_screenshot(const Image<ColorRgb>&)
///                      signal.
/// @param opts          Grabber specific options, must provide the common fields
///                      video3DSBS, video3DTAB, screenshot, address, priority and
///                      skipReply.
///
template <typename WrapperType, typename OptionsType>
int runFlatbufferScreenGrabber(const QString& grabberName,
                               WrapperType& grabber,
                               const OptionsType& opts,
                               QSharedPointer<Logger> log,
                               ErrorManager& errorManager)
{
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
		grabberName + QStringLiteral(" Standalone"),
		hostName,
		opts.priority,
		opts.skipReply,
		static_cast<quint16>(port));

	// Connect the screen capturing to flatbuf connection processing
	QObject::connect(&grabber, &WrapperType::sig_screenshot,
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
