#include "websockets.h"

WebSockets::WebSockets(QTcpSocket *socket, JsonProcessor *jsonProcessor)
	: _socket(socket)
	, _jsonProcessor(jsonProcessor)
{
}

WebSockets::~WebSockets()
{
}
