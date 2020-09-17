#include "WebJsonRpc.h"
#include "QtHttpReply.h"
#include "QtHttpRequest.h"
#include "QtHttpServer.h"
#include "QtHttpClientWrapper.h"

#include <api/JsonAPI.h>

WebJsonRpc::WebJsonRpc(QtHttpRequest* request, QtHttpServer* server, bool localConnection, QtHttpClientWrapper* parent)
	: QObject(parent)
	, _server(server)
	, _wrapper(parent)
	, _log(Logger::getInstance("HTTPJSONRPC"))
{
	const QString client = request->getClientInfo().clientAddress.toString();
	_jsonAPI = new JsonAPI(client, _log, localConnection, this, true);
	connect(_jsonAPI, &JsonAPI::callbackMessage, this, &WebJsonRpc::handleCallback);
	connect(_jsonAPI, &JsonAPI::forceClose, [&]() { _wrapper->closeConnection(); _stopHandle = true; });
	_jsonAPI->initialize();
}

void WebJsonRpc::handleMessage(QtHttpRequest* request)
{
	// TODO better solution. If jsonAPI emits forceClose the request is deleted and the following call to this method results in segfault
	if(!_stopHandle)
	{
		QByteArray header = request->getHeader("Authorization");
		QByteArray data = request->getRawData();
		_unlocked = true;
		_jsonAPI->handleMessage(data,header);
	}
}

void WebJsonRpc::handleCallback(QJsonObject obj)
{
	// guard against wrong callbacks; TODO: Remove when JSONAPI is more solid
	if(!_unlocked) return;
	_unlocked = false;
	// construct reply with headers timestamp and server name
	QtHttpReply reply(_server);
	QJsonDocument doc(obj);
	reply.addHeader ("Content-Type", "application/json");
	reply.appendRawData (doc.toJson());
	_wrapper->sendToClientWithReply(&reply);
}
