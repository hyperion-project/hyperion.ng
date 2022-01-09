#pragma once

// util
#include <utils/Logger.h>
#include <utils/settings.h>

// qt
#include <QVector>

class QTcpServer;
class FlatBufferClient;
class NetOrigin;


///
/// @brief A TcpServer to receive images of different formats with Google Flatbuffer
/// Images will be forwarded to all Hyperion instances
///
class FlatBufferServer : public QObject
{
	Q_OBJECT

public:
	FlatBufferServer(const QJsonDocument& config, QObject* parent = nullptr);
	~FlatBufferServer() override;

public slots:
	///
	/// @brief Handle settings update
	/// @param type   The type from enum
	/// @param config The configuration
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	void initServer();

signals:
	///
	/// @emits whenever the server would like to announce its service details
	///
	void publishService(const QString& serviceType, quint16 servicePort, const QByteArray& serviceName = "");

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

	QVector<FlatBufferClient*> _openConnections;
};
