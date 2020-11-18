#pragma once

#include <utils/Logger.h>

class QUdpSocket;

///
/// @brief The SSDP Server sends and receives (parses) SSDP requests
///
class SSDPServer : public QObject
{
	Q_OBJECT

public:
	friend class SSDPHandler;
	///
	/// @brief Construct the server, listen on default ssdp address/port with multicast
	/// @param parent  The parent object
	///
	SSDPServer(QObject* parent = nullptr);
	~SSDPServer() override;

	///
	/// @brief Prepare server after thread start
	///
	void initServer();

	///
	/// @brief Start SSDP
	/// @return false if already running or bind failure
	///
	bool start();

	///
	/// @brief Stop SSDP
	///
	void stop();

	///
	/// @brief Send an answer to mSearch requester
	/// @param st         the searchTarget
	/// @param senderIp   Ip address of the sender
	/// @param senderPort The port of the sender
	///
	void sendMSearchResponse(const QString& st, const QString& senderIp, quint16 senderPort);

	///
	/// @brief Send ByeBye notification (on SSDP stop) (repeated 3 times)
	/// @param st        Search target
	///
	void sendByeBye(const QString& st);

	///
	/// @brief Send a NOTIFY msg on SSDP startup to notify our presence (repeated 3 times)
	/// @param st        The search target
	///
	void sendAlive(const QString& st);

	///
	/// @brief Send a NOTIFY msg as ssdp:update to notify about changes
	/// @param st        The search target
	///
	void sendUpdate(const QString& st);

	///
	/// @brief Overwrite description address
	/// @param addr  new address
	///
	void setDescriptionAddress( const QString &addr ) { _descAddress = addr; }

	///
	/// @brief Set uuid
	/// @param uuid  The uuid
	///
	void setUuid( const QString &uuid ) { _uuid = uuid; }

	///
	/// @brief set new flatbuffer server port
	///
	void setFlatBufPort( quint16 port ){_fbsPort = QString::number( port ); }

	///
	/// @brief Get current flatbuffer server port
	///
	quint16 getFlatBufPort() const
	{
		return _fbsPort.toInt();
	}
	///
	/// @brief set new protobuf server port
	///
	void setProtoBufPort( quint16 port ) { _pbsPort = QString::number( port ); }

	///
	/// @brief Get current protobuf server port
	///
	quint16 getProtoBufPort() const { return _pbsPort.toInt(); }
	///
	/// @brief set new json server port
	///
	void setJsonServerPort( quint16 port ) { _jssPort = QString::number( port ); }
	///
	/// @brief get new json server port
	///
	quint16 getJsonServerPort() const { return _jssPort.toInt(); }
	///
	/// @brief set new ssl server port
	///
	void setSSLServerPort( quint16 port ) { _sslPort = QString::number( port ); }
	///
	/// @brief get new ssl server port
	///
	quint16 getSSLServerPort() const { return _sslPort.toInt(); }

	///
	/// @brief set new hyperion name
	///
	void setHyperionName( const QString &name ) { _name = name; }

	///
	/// @brief get hyperion name
	///
	QString getHyperionName() const { return _name; }
	
signals:
	///
	/// @brief Emits whenever a new SSDP search "man : ssdp:discover" is received along with the service type
	/// @param target  The ST service type
	/// @param mx      Answer with delay in s
	/// @param address The ip of the caller
	/// @param port    The port of the caller
	///
	void msearchRequestReceived( const QString &target,
								 const QString &mx,
								 const QString address,
								 quint16 port );

private:
	Logger *_log;
	QUdpSocket *_udpSocket;

	QString _serverHeader, _uuid, _fbsPort, _pbsPort, _jssPort, _sslPort, _name, _descAddress;
	bool _running;

private slots:
	void readPendingDatagrams();
};
