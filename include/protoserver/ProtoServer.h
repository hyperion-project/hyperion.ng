#pragma once

// util
#include <utils/Logger.h>
#include <utils/settings.h>

// qt
#include <QVector>

class QTcpServer;
class ProtoClientConnection;
class NetOrigin;

///
/// @brief This class creates a TCP server which accepts connections wich can then send
/// in Protocol Buffer encoded commands. This interface to Hyperion is used by various
/// third-party applications
///
class ProtoServer : public QObject
{
	Q_OBJECT

public:
	ProtoServer(const QJsonDocument& config, QObject* parent = nullptr);
	~ProtoServer() override;

signals:
	///
	/// @emits whenever the server would like to announce its service details
	///
	void publishService(const QString& serviceType, quint16 servicePort, const QByteArray& serviceName = "");

public slots:
	///
	/// @brief Handle settings update
	/// @param type   The type from enum
	/// @param config The configuration
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	void initServer();

private slots:
	///
	/// @brief Is called whenever a new socket wants to connect
	///
	void newConnection();

	///
	/// @brief is called whenever a client disconnected
	///
	void clientDisconnected();

private:
	///
	/// @brief Start the server with current _port
	///
	void startServer();

	///
	/// @brief Stop server
	///
	void stopServer();


private:
	QTcpServer* _server;
	NetOrigin* _netOrigin;
	Logger* _log;
	int _timeout;
	quint16 _port;
	const QJsonDocument _config;

	QVector<ProtoClientConnection*> _openConnections;
};
