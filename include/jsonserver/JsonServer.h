#pragma once

// system includes
#include <cstdint>

// Qt includes
#include <QTcpServer>
#include <QSet>

// Hyperion includes
#include <hyperion/Hyperion.h>

class JsonClientConnection;

class JsonServer : public QObject
{
	Q_OBJECT

public:
	JsonServer(Hyperion * hyperion, uint16_t port = 19444);
	~JsonServer();

	uint16_t getPort() const;

private slots:
	void newConnection();

	void closedConnection(JsonClientConnection * connection);

private:
	Hyperion * _hyperion;

	QTcpServer _server;

	QSet<JsonClientConnection *> _openConnections;
};
