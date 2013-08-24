
#pragma once

// system includes
#include <cstdint>
#include <string>

// QT includes
#include <QTimer>
#include <QString>
#include <QTcpSocket>
#include <QByteArray>

// Hyperion includes
#include <hyperion/Hyperion.h>

/// Check if XBMC is playing something. When it does not, this class will send all black data Hyperion to
/// override (grabbed) data with a lower priority
///
/// Note: The json TCP server needs to be enabled manually in XBMC in System/Settings/Network/Services
class XBMCVideoChecker : public QObject
{
Q_OBJECT

public:
	XBMCVideoChecker(const std::string & address, uint16_t port, uint64_t interval, Hyperion * hyperion, int priority);

	void start();

private slots:
	void sendRequest();

	void receiveReply();

private:
	const QString _address;

	const uint16_t _port;

	const QByteArray _request;

	QTimer _timer;

	QTcpSocket _socket;

	Hyperion * _hyperion;

	const int _priority;
};

