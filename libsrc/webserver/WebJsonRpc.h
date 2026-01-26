#pragma once

#include <QJsonObject>
#include <QLoggingCategory>

#include <utils/Logger.h>
#include "QtHttpReply.h"

Q_DECLARE_LOGGING_CATEGORY(comm_webjsonrpc_track);
Q_DECLARE_LOGGING_CATEGORY(comm_webjsonrpc_send);
Q_DECLARE_LOGGING_CATEGORY(comm_webjsonrpc_receive);

class QtHttpServer;
class QtHttpRequest;
class QtHttpClientWrapper;
class JsonAPI;

class WebJsonRpc : public QObject {
	Q_OBJECT
public:
	WebJsonRpc(QtHttpClientWrapper* wrapper, const QtHttpRequest* request, QtHttpServer* server, bool localConnection);
	~WebJsonRpc() override;

	void handleMessage(const QtHttpRequest* request);

private:
	QtHttpServer* _httpServer;
	QtHttpClientWrapper* _httpClientHandler;
	QSharedPointer<Logger> _log;
	QSharedPointer<JsonAPI> _jsonAPI;

	bool _stopHandle = false;
	bool _unlocked = false;

private slots:
	void sendOkCallbackMessage(QJsonObject obj);
	void onForbidden();

private:
	void sendCallbackMessage(QJsonObject obj, QtHttpReply::StatusCode code);
};
