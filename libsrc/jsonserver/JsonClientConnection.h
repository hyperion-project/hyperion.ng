#pragma once

// stl includes
#include <string>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>

// jsoncpp includes
#include <json/json.h>

class JsonClientConnection : public QObject
{
	Q_OBJECT

public:
	JsonClientConnection(QTcpSocket * socket);
	~JsonClientConnection();

signals:
	void connectionClosed(JsonClientConnection * connection);

private slots:
	void readData();
	void socketClosed();

private:
	void handleMessage(const std::string & message);
	void handleNotImplemented(const Json::Value & message);

	void sendMessage(const Json::Value & message);
	void sendErrorReply(const std::string & error);

private:
	QTcpSocket * _socket;

	QByteArray _receiveBuffer;
};
