#include "WebJsonRpc.h"
#include "QtHttpReply.h"
#include "QtHttpRequest.h"
#include "QtHttpServer.h"
#include "QtHttpClientWrapper.h"

#include <api/JsonAPI.h>

WebJsonRpc::WebJsonRpc(QtHttpRequest* request, QtHttpServer* server, QtHttpClientWrapper* parent)
	: QObject(parent)
	, _server(server)
	, _wrapper(parent)
	, _log(Logger::getInstance("HTTPJSONRPC"))
{
	const QString client = request->getClientInfo().clientAddress.toString();
	_jsonAPI = new JsonAPI(client, _log, this, true);
	connect(_jsonAPI, &JsonAPI::callbackMessage, this, &WebJsonRpc::handleCallback);
}

void WebJsonRpc::handleMessage(QtHttpRequest* request)
{
	QByteArray data = request->getRawData();
	_unlocked = true;
	_jsonAPI->handleMessage(data);
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
