#pragma once

// stl includes
#include <string>

// Qt includes
#include <QByteArray>
#include <QTcpSocket>

// jsoncpp includes
#include <json/json.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

// util includes
#include <utils/jsonschema/JsonSchemaChecker.h>

class JsonClientConnection : public QObject
{
	Q_OBJECT

public:
	JsonClientConnection(QTcpSocket * socket, Hyperion * hyperion);
	~JsonClientConnection();

signals:
	void connectionClosed(JsonClientConnection * connection);

private slots:
	void readData();
	void socketClosed();

private:
	void handleMessage(const std::string & message);
	void handleColorCommand(const Json::Value & message);
	void handleImageCommand(const Json::Value & message);
	void handleServerInfoCommand(const Json::Value & message);
	void handleClearCommand(const Json::Value & message);
	void handleClearallCommand(const Json::Value & message);
	void handleTransformCommand(const Json::Value & message);
	void handleNotImplemented();

	void sendMessage(const Json::Value & message);
	void sendSuccessReply();
	void sendErrorReply(const std::string & error);

private:
	bool checkJson(const Json::Value & message, const QString &schemaResource, std::string & errors);

private:
	QTcpSocket * _socket;

	Hyperion * _hyperion;

	QByteArray _receiveBuffer;
};
