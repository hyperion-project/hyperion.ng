#pragma once

#include <utils/Logger.h>

#include <QJsonObject>

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
	Logger* _log;
	JsonAPI* _jsonAPI;

	bool _stopHandle = false;
	bool _unlocked = false;

private slots:
	void handleCallback(QJsonObject obj);
};
