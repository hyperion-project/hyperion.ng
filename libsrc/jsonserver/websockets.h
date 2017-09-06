#pragma once

#include <QTcpSocket>
#include <QString>
#include <QByteArray>

#include <utils/Logger.h>
#include <utils/JsonProcessor.h>

class WebSockets
{
public:
	WebSockets(QTcpSocket *socket, JsonProcessor *jsonProcessor);
	~WebSockets();
	
private:
	QTcpSocket * _socket;
	JsonProcessor * _jsonProcessor;
	QByteArray _buffer;
};


