#pragma once

// Qt includes
#include <QString>
#include <QByteArray>

// util includes
#include <utils/Logger.h>

class JsonProcessor;
class QTcpSocket;

///
/// The Connection object created by \a JsonServer when a new connection is establshed
///
class JsonClientConnection : public QObject
{
	Q_OBJECT

public:
	///
	/// Constructor
	/// @param socket The Socket object for this connection
	///
	JsonClientConnection(QTcpSocket * socket);

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
	/// new instance of JsonProcessor
	JsonProcessor * _jsonProcessor;

	/// The buffer used for reading data from the socket
	QByteArray _receiveBuffer;

	/// The logger instance
	Logger * _log;
};
