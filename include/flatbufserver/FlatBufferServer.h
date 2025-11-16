#pragma once

// util
#include <utils/Logger.h>
#include <utils/settings.h>

// qt
#include <QVector>
#include <QScopedPointer>
#include <QWeakPointer>

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
	explicit FlatBufferServer(const QJsonDocument& config, QObject* parent = nullptr);
	~FlatBufferServer() override;

public slots:
	///
	/// @brief Handle settings update
	/// @param type   The type from enum
	/// @param config The configuration
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	void initServer();

	///
	/// @brief Stop server
	///
	void stop();

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
	void start();

private:
	QScopedPointer<QTcpServer> _server;
	QWeakPointer<NetOrigin> _netOriginWeak;
	QSharedPointer<Logger> _log;
	int _timeout;
	quint16 _port;
	const QJsonDocument _config;

	int _pixelDecimation;

	QVector<FlatBufferClient*> _openConnections;
};
