#pragma once

#include <QJsonObject>

#include <utils/Logger.h>
#include "QtHttpReply.h"

class QtHttpServer;
class QtHttpRequest;
class QtHttpClientWrapper;
class JsonAPI;

class WebJsonRpc : public QObject {
	Q_OBJECT
public:
	WebJsonRpc(QtHttpRequest* request, QtHttpServer* server, bool localConnection, QtHttpClientWrapper* parent);

	void handleMessage(QtHttpRequest* request);

private:
	QtHttpServer* _server;
	QtHttpClientWrapper* _wrapper;
	QSharedPointer<Logger> _log;
	JsonAPI* _jsonAPI;

	bool _stopHandle = false;
	bool _unlocked = false;

private slots:
	void sendOkCallbackMessage(QJsonObject obj);
	void onForbidden();

private:
	void sendCallbackMessage(QJsonObject obj, QtHttpReply::StatusCode code);
};
