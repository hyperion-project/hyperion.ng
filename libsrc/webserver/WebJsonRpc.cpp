#include "WebJsonRpc.h"
#include "QtHttpReply.h"
#include "QtHttpRequest.h"
#include "QtHttpServer.h"
#include "QtHttpClientWrapper.h"

#include <api/JsonAPI.h>
#include <api/JsonCallbacks.h>

Q_LOGGING_CATEGORY(comm_webjsonrpc_track, "hyperion.comm.webjsonrpc.track");
Q_LOGGING_CATEGORY(comm_webjsonrpc_receive, "hyperion.comm.webjsonrpc.receive");
Q_LOGGING_CATEGORY(comm_webjsonrpc_send, "hyperion.comm.webjsonrpc.send");

WebJsonRpc::WebJsonRpc(QtHttpClientWrapper* wrapper, const QtHttpRequest* request, QtHttpServer* server, bool localConnection)
	: QObject()
	, _httpServer(server)
	, _httpClientHandler(wrapper)
	, _log(Logger::getInstance("HTTPJSONRPC"))
{
	TRACK_SCOPE();
	const QString client = request->getClientInfo().clientAddress.toString();
	_jsonAPI = MAKE_TRACKED_SHARED(JsonAPI, client, _log, localConnection, true);
	connect(_jsonAPI.get(), &JsonAPI::callbackReady, this, &WebJsonRpc::sendOkCallbackMessage);

	connect(_jsonAPI.get(), &JsonAPI::isForbidden, this, &WebJsonRpc::onForbidden);
	connect(_jsonAPI->getCallBack().get(), &JsonCallbacks::callbackReady, this, &WebJsonRpc::sendOkCallbackMessage);

	_jsonAPI->initialize();
}

WebJsonRpc::~WebJsonRpc()
{
	TRACK_SCOPE();
}

void WebJsonRpc::handleMessage(const QtHttpRequest* request)
{
	// TODO better solution. If jsonAPI emits forceClose the request is deleted and the following call to this method results in segfault
	if(!_stopHandle)
	{
		QByteArray header = request->getHeader("Authorization");
		QByteArray data = request->getRawData();
		_unlocked = true;
		qCDebug(comm_webjsonrpc_receive) << "WebJsonRpc message received:" << data << ", plus header:" << header;
		_jsonAPI->handleMessage(data,header);
	}
}

void WebJsonRpc::onForbidden()
{
	qCDebug(comm_webjsonrpc_track) << "WebJsonRpc connection is forbidden, closing connection";

	QJsonObject response;
	response["success"] = false;
	response["error"] = "password change required";
	response["errorData"] = QJsonArray{QJsonObject({{"description", "Default password must be changed before accessing the API"}})};

	_unlocked = true; // unlock to allow sending the response
	sendCallbackMessage(response, QtHttpReply::StatusCode::Forbidden);

	_httpClientHandler->closeConnection();
	_stopHandle = true; 
}

void WebJsonRpc::sendOkCallbackMessage(QJsonObject obj)
{
	sendCallbackMessage(obj, QtHttpReply::Ok);
}

void WebJsonRpc::sendCallbackMessage(QJsonObject obj, QtHttpReply::StatusCode code)
{
	// guard against wrong callbacks; TODO: Remove when JSONAPI is more solid
	if(!_unlocked) return;
	_unlocked = false;
	// construct reply with headers timestamp and server name
	QtHttpReply reply(_httpServer);
	reply.setStatusCode(code);
	reply.addHeader ("Content-Type", "application/json");
	QByteArray data = QJsonDocument(obj).toJson();
	reply.appendRawData (data);
	qCDebug(comm_webjsonrpc_send) << "WebJsonRpc send message [" << code << "]:" << obj;
	_httpClientHandler->sendToClientWithReply(&reply);
}
