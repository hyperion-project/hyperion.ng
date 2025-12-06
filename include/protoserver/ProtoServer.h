#pragma once

#include <QVector>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QWeakPointer>
#include <QLoggingCategory>

#include <utils/Logger.h>
#include <utils/settings.h>

Q_DECLARE_LOGGING_CATEGORY(proto_server_flow);

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
	explicit ProtoServer(const QJsonDocument& config, QObject* parent = nullptr);
	~ProtoServer() override;

	///
	/// @brief Register all connected clients again (e.g. after a Hyperion instance restart)
	///	
	void registerClients() const;

signals:
	///
	/// @emits whenever the server would like to announce its service details
	///
	void publishService(const QString& serviceType, quint16 servicePort, const QByteArray& serviceName = "");

	///
	/// @emits when the ProtoServer has completed its stop/cleanup
	///
	void isStopped();

public slots:
	///
	/// @brief Handle settings update
	/// @param type   The type from enum
	/// @param config The configuration
	///
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

	void initServer();

	///
	/// @brief Open server for connections
	///
	void open();

	///
	/// @brief Close server connections
	///
	void close();

	///
	/// @brief Stop server
	///
	void stop();

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
	void start() const;

private:
	QScopedPointer<QTcpServer> _server;
	QWeakPointer<NetOrigin> _netOriginWeak;
	QSharedPointer<Logger> _log;
	int _timeout;
	quint16 _port;
	const QJsonDocument _config;

	QVector<QSharedPointer<ProtoClientConnection>> _openConnections;
};
