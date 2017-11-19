#pragma once

#include <utils/Logger.h>

class QtHttpServer;
class QtHttpRequest;
class QtHttpClientWrapper;
class JsonProcessor;

class WebJsonRpc : public QObject {
	Q_OBJECT
public:
	WebJsonRpc(QtHttpRequest* request, QtHttpServer* server, QtHttpClientWrapper* parent);

	void handleMessage(QtHttpRequest* request);

private:
	QtHttpServer* _server;
	QtHttpClientWrapper* _wrapper;
	Logger* _log;
	JsonProcessor* _jsonProcessor;

	bool _unlocked = false;

private slots:
	void handleCallback(QJsonObject obj);
};
