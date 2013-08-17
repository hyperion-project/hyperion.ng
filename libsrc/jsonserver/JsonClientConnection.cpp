// stl includes
#include <iostream>

#include "JsonClientConnection.h"

JsonClientConnection::JsonClientConnection(QTcpSocket *socket) :
	QObject(),
	_socket(socket)
{
	connect(_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
	connect(_socket, SIGNAL(readyRead()), this, SLOT(readData()));
}


JsonClientConnection::~JsonClientConnection()
{
	delete _socket;
}

void JsonClientConnection::readData()
{
	_receiveBuffer += _socket->readAll();

	int bytes = _receiveBuffer.indexOf('\n') + 1;
	if (bytes != 0)
	{
		// create message string
		std::string message(_receiveBuffer.data(), bytes);

		// remove message data from buffer
		_receiveBuffer = _receiveBuffer.mid(bytes);

		// handle message
		handleMessage(message);
	}
}

void JsonClientConnection::socketClosed()
{
	emit connectionClosed(this);
}

void JsonClientConnection::handleMessage(const std::string &message)
{
	Json::Reader reader;
	Json::Value messageRoot;
	if (!reader.parse(message, messageRoot, false))
	{
		sendErrorReply("Error while parsing json: " + reader.getFormattedErrorMessages());
		return;
	}

	handleNotImplemented(messageRoot);
}

void JsonClientConnection::handleNotImplemented(const Json::Value & message)
{
	sendErrorReply("Command not implemented");
}

void JsonClientConnection::sendMessage(const Json::Value &message)
{
	Json::FastWriter writer;
	std::string serializedReply = writer.write(message);
	_socket->write(serializedReply.data(), serializedReply.length());
}

void JsonClientConnection::sendErrorReply(const std::string &error)
{
	// create reply
	Json::Value reply;
	reply["success"] = false;
	reply["error"] = error;

	// send reply
	sendMessage(reply);
}
