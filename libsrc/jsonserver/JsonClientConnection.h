#pragma once

// Qt includes
#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QHostAddress>

// util includes
#include <utils/Logger.h>

class JsonAPI;
class QTcpSocket;

///
/// The Connection object created by \a JsonServer when a new connection is established
///
class JsonClientConnection : public QObject
{
	Q_OBJECT

public:
	///
	/// Constructor
	/// @param socket The Socket object for this connection
	///
	JsonClientConnection(QTcpSocket * socket, bool localConnection);
	QHostAddress getClientAddress();

signals:
	void connectionClosed();

public slots:
	qint64 sendMessage(QJsonObject);

private slots:
	///
	/// Slot called when new data has arrived
	///
	void readRequest();

	void disconnected();

private:
	QTcpSocket* _socket;
	/// new instance of JsonAPI
	JsonAPI * _jsonAPI;

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;

	/// The logger instance
	Logger * _log;
};
